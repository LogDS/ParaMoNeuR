//
 #include <cuda_runtime.h>
 #include <driver_types.h>
// #include <iostream>
// #include <vector>
// #include <fstream>
// #include <sstream>
//
// #include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"
//
//
// void run_scalability_experiment(size_t current_nnz, std::ofstream& csv_file) {
//     std::vector<uint64_t> shape_5d = {50, 50, 50, 20, 10};
//     size_t fixed_capacity = static_cast<size_t>(current_nnz * 1.4);
//
//     cudaEvent_t start_mut, stop_mut, start_alg, stop_alg;
//     cudaEventCreate(&start_mut); cudaEventCreate(&stop_mut);
//     cudaEventCreate(&start_alg); cudaEventCreate(&stop_alg);
//
//     DynamicHybridTensor T_A(shape_5d, fixed_capacity);
//     DynamicHybridTensor T_B(shape_5d, fixed_capacity);
//
//     logds::types::SparseTensor cpu_A("benchmark_data/tensor_A_" + std::to_string(current_nnz) + ".csv",0.0 );
//     logds::types::SparseTensor  cpu_B("benchmark_data/tensor_B_" + std::to_string(current_nnz) + ".csv", 0.0);
//
//     // 1. MISURA MUTAZIONE (Inizializzazione da file paritetici)
//     cudaEventRecord(start_mut);
//     T_A.initialize_from_cpu(cpu_A);
//     T_B.initialize_from_cpu(cpu_B);
//     cudaEventRecord(stop_mut);
//     cudaEventSynchronize(stop_mut);
//     float t_mutation_ms = 0;
//     cudaEventElapsedTime(&t_mutation_ms, start_mut, stop_mut);
//
//     T_A.consolidate();
//     T_B.consolidate();
//
//     // 2. MISURA ALGEBRA PESANTE (EINSUM)
//     cudaEventRecord(start_alg);
//     EinsumRecipe recipe;
//     recipe.input_axes = {{0, 1, 2, 3, 4}, {0, 1, 2, 3, 4}};
//     recipe.output_axes = {0, 4};
//     std::vector<DynamicHybridTensor*> nargs{&T_A, &T_B};
//     DynamicHybridTensor Out_Einsum = DynamicHybridTensor::n_way_einstein_sum(nargs, recipe, {50, 10}, fixed_capacity);
//
//     cudaEventRecord(stop_alg);
//     cudaEventSynchronize(stop_alg);
//     float t_algebra_ms = 0;
//     cudaEventElapsedTime(&t_algebra_ms, start_alg, stop_alg);
//
//     // --- BARRIERA DI CONTROLLO ANTI-OTTIMIZZAZIONE (DCE BLOCK) ---
//     // Scarichiamo il primo float calcolato dall'Einsum verso la CPU.
//     // Questo rende l'esito del kernel "necessario", distruggendo la Dead Code Elimination di nvcc!
//     float h_anti_dce_verification = 0.0f;
//     cudaMemcpy(&h_anti_dce_verification, Out_Einsum.d_csf_vals, sizeof(float), cudaMemcpyDeviceToHost);
//
//     // Scrittura dei log reali ed effettivi
//     csv_file << "Paramoneur," << current_nnz << "," << t_mutation_ms << "," << t_algebra_ms << "\n";
//     std::cout << "[REAL RUN NNZ: " << current_nnz << "] Mutazione: " << t_mutation_ms << " ms | Algebra: " << t_algebra_ms << " ms | Controllo Hardware: " << h_anti_dce_verification << "\n";
//
//     cudaEventDestroy(start_mut); cudaEventDestroy(stop_mut);
//     cudaEventDestroy(start_alg); cudaEventDestroy(stop_alg);
// }
//
// int main() {
//     int deviceCount = 0;
//     cudaGetDeviceCount(&deviceCount);
//     std::ofstream csv_file("benchmark_custom_engine.csv");
//     csv_file << "NNZ,Tempo_Mutazione_ms,Tempo_Algebra_ms\n";
//
//     std::vector<size_t> nnz_scale_steps = {1000, 5000, 25000, 100000, 500000};
//
//     for (size_t current_nnz : nnz_scale_steps) {
//         run_scalability_experiment(current_nnz, csv_file);
//     }
//
//     csv_file.close();
//     return 0;
// }

#include <iostream>
#include <vector>
#include <c++/13/cassert>

#include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"

// #define CUDA_CHECK(ans) { gpuAssert((ans), __FILE__, __LINE__); }
// inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true) {
//    if (code != cudaSuccess) {
//       fprintf(stderr, "GPU Error: %s %s %d\n", cudaGetErrorString(code), file, line);
//       if (abort) exit(code);
//    }
// }

#include <string>
#include <stdexcept>
#include <cuda/std/detail/libcxx/include/cstring>

EinsumPlan compile_einsum_string_simple(const std::string& expression) {
    EinsumPlan plan{};

    // 1. Inizializzazione degli array statici (tutti a 0 per coerenza con il tuo schema)
    plan.tensor_order = 0;
    plan.num_contracted_modes = 0;
    for (int i = 0; i < MAX_TENSOR_ORDER; ++i) {
        plan.contracted_levels[i] = 0;
        plan.dense_dim_strides[i] = 0;
    }

    // 2. Splitting minimale della stringa per isolare i componenti
    size_t comma_pos = expression.find(',');
    size_t arrow_pos = expression.find("->");

    if (comma_pos == std::string::npos || arrow_pos == std::string::npos) {
        throw std::invalid_argument("Formato non valido. Usa il formato 'ijko,o->ijk'");
    }

    std::string sparse_labels = expression.substr(0, comma_pos);
    std::string output_labels = expression.substr(arrow_pos + 2);

    // Imposta il rango del tensore sparse
    plan.tensor_order = static_cast<int>(sparse_labels.length());

    // 3. Trova le modalità contratte (presenti in sparse ma NON in output)
    for (int i = 0; i < plan.tensor_order; ++i) {
        char current_label = sparse_labels[i];

        // Se la label non è presente nell'output, la contraiamo
        if (output_labels.find(current_label) == std::string::npos) {
            // Mappa l'indice del livello e imposta lo stride fisso a 1
            plan.contracted_levels[plan.num_contracted_modes] = i;
            plan.dense_dim_strides[plan.num_contracted_modes] = 1;

            plan.num_contracted_modes++;
        }
    }

    return plan;
}

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cassert>
#include <map>
#include <tuple>
// #include "csf_einsum.cuh"

#define CUDA_CHECK(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true) {
   if (code != cudaSuccess) {
      fprintf(stderr, "GPU Error: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}
//
// // Primitiva di compilazione sviluppata nei passi precedenti
// EinsumPlan compile_einsum_string_simple(const std::string& expression) {
//     EinsumPlan plan{};
//     plan.tensor_order = 0;
//     plan.num_contracted_modes = 0;
//     for (int i = 0; i < MAX_TENSOR_ORDER; ++i) {
//         plan.contracted_levels[i] = 0;
//         plan.dense_dim_strides[i] = 0;
//     }
//
//     size_t comma_pos = expression.find(',');
//     size_t arrow_pos = expression.find("->");
//     if (comma_pos == std::string::npos || arrow_pos == std::string::npos) {
//         throw std::invalid_argument("Formato stringa Einsum non valido.");
//     }
//
//     std::string sparse_labels = expression.substr(0, comma_pos);
//     std::string output_labels = expression.substr(arrow_pos + 2);
//     plan.tensor_order = static_cast<int>(sparse_labels.length());
//
//     for (int i = 0; i < plan.tensor_order; ++i) {
//         char current_label = sparse_labels[i];
//         if (output_labels.find(current_label) == std::string::npos) {
//             plan.contracted_levels[plan.num_contracted_modes] = i;
//             plan.dense_dim_strides[plan.num_contracted_modes] = 1;
//             plan.num_contracted_modes++;
//         }
//     }
//     return plan;
// }

int main() {
    std::cout << "[DRIVER FIX] Avvio dello Stress Test con inizializzazione degli array statici..." << std::endl;

    int dense_sizes[] = {2};
    std::string expression = "ijko,o->ijk";
    EinsumPlan plan = compile_einsum_string_simple(expression);

    int root_size = 2; // i = 0, 1
    UniversalCSF csf{};
    csf.order = 4;
    csf.nnz = 16;

    // =========================================================================
    // CRITICO: Pre-inizializzazione esplicita di tutti i 7 livelli hardware statici
    // =========================================================================
    for (int i = 0; i < MAX_TENSOR_ORDER; ++i) {
        csf.ptr_offsets[i] = 0;
        csf.ind_offsets[i] = 0;
    }

    // Struttura ad albero geometricamente solida e allineata
    std::vector<int> h_all_ptrs = {
        0, 2, 4,                        // Livello 0 (i): Dimensione = 3
        0, 2, 4, 6, 8,                  // Livello 1 (j): Dimensione = 5
        0, 2, 4, 6, 8, 10, 12, 14, 16   // Livello 2 (k): Dimensione = 9
    };

    std::vector<int> h_all_indices = {
        0, 1,                                           // Livello 0 (i): size = 2
        0, 1, 0, 1,                                     // Livello 1 (j): size = 4
        0, 1, 0, 1, 0, 1, 0, 1,                         // Livello 2 (k): size = 8
        0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1  // Livello 3 (o): size = 16
    };

    std::vector<float> h_vals(csf.nnz);
    for(int i = 0; i < csf.nnz; ++i) h_vals[i] = (float)(i + 1);

    // Assegnazione indicizzata esplicita e corretta degli offset strutturali
    csf.ptr_offsets[0] = 0;
    csf.ptr_offsets[1] = 3;
    csf.ptr_offsets[2] = 8;
    csf.ptr_offsets[3] = 0; // Il livello foglia non ha puntatori a figli successivi

    csf.ind_offsets[0] = 0;
    csf.ind_offsets[1] = 2;
    csf.ind_offsets[2] = 6;
    csf.ind_offsets[3] = 14;

    std::vector<float> h_dense_in = {2.0f, 5.0f};

    // Costruzione mappa CPU di riferimento per il controllo incrociato
    std::map<std::vector<int>, float> cpu_map;
    int val_idx = 0;
    for(int i = 0; i < 2; ++i) {
        for(int j = 0; j < 2; ++j) {
            for(int k = 0; k < 2; ++k) {
                float sum = 0.0f;
                for(int o = 0; o < 2; ++o) {
                    sum += h_vals[val_idx] * h_dense_in[o];
                    val_idx++;
                }
                cpu_map[{i, j, k}] = sum;
            }
        }
    }

    // Allocazione e trasferimento su GPU
    CUDA_CHECK(cudaMalloc(&csf.d_all_ptrs, h_all_ptrs.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&csf.d_all_indices, h_all_indices.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(csf.d_all_ptrs, h_all_ptrs.data(), h_all_ptrs.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(csf.d_all_indices, h_all_indices.data(), h_all_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));

    float* d_dense_in;
    CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));

    int out_order = csf.order - plan.num_contracted_modes;
    int* d_out_indices; float* d_out_vals; int* d_global_counter;

    CUDA_CHECK(cudaMalloc(&d_out_indices, csf.nnz * out_order * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out_vals, csf.nnz * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
    CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));

    // Esecuzione del kernel
    launch_csf_einsum(csf, plan, d_dense_in, root_size, d_out_indices, d_out_vals, d_global_counter);
    CUDA_CHECK(cudaDeviceSynchronize());

    int h_final_nnz = 0;
    CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));

    std::cout << " -> Elementi unici contratti estratti da GPU: " << h_final_nnz << std::endl;

    if (h_final_nnz == 0) {
        std::cout << "[ERROR] La GPU non ha trovato elementi foglia. Verificare i puntatori." << std::endl;
        return -1;
    }

    std::vector<int> h_out_indices(h_final_nnz * out_order);
    std::vector<float> h_out_vals(h_final_nnz);

    CUDA_CHECK(cudaMemcpy(h_out_indices.data(), d_out_indices, h_final_nnz * out_order * sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_out_vals.data(), d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));

    // Validazione dei risultati numerici
    bool pass = true;
    for(int n = 0; n < h_final_nnz; ++n) {
        std::vector<int> coord = {h_out_indices[n * out_order], h_out_indices[n * out_order + 1], h_out_indices[n * out_order + 2]};
        if(std::abs(h_out_vals[n] - cpu_map[coord]) > 1e-3f) {
            pass = false;
            std::cout << "Mismatch alla coordinata (" << coord[0] << "," << coord[1] << "," << coord[2]
                      << ") -> GPU: " << h_out_vals[n] << ", CPU: " << cpu_map[coord] << std::endl;
        }
    }

    if(pass && h_final_nnz == cpu_map.size()) {
        std::cout << "[SUCCESS] Inizializzazione corretta! Tutti i vettori statici sono allineati. Test passato!" << std::endl;
    } else {
        std::cout << "[FAIL] Errore di validazione algebrica." << std::endl;
    }

    cudaFree(csf.d_all_ptrs); cudaFree(csf.d_all_indices); cudaFree(csf.d_vals);
    cudaFree(d_dense_in); cudaFree(d_out_indices); cudaFree(d_out_vals); cudaFree(d_global_counter);
    return 0;
}

//
// int main() {
// #if 0
//     std::cout << "[STRESS TEST] Generazione di una struttura CSF ad alto volume..." << std::endl;
//
//     std::string expression = "ijko,o->ijk";
//     EinsumPlan plan = compile_einsum_string_simple(expression);
//
//     int root_size = 4;
//     std::vector<int> h_all_ptrs;
//     std::vector<int> h_all_indices;
//     std::vector<float> h_vals;
//
//     // Struttura dati CPU per accumulare il riferimento corretto nello spazio 3D contratto (i,j,k)
//     typedef std::tuple<int, int, int> Coord3D;
//     std::map<Coord3D, float> cpu_reference_map;
//
//     h_all_ptrs.push_back(0);
//     int j_counter = 0;
//     int k_counter = 0;
//     int o_counter = 0;
//
//     std::vector<int> lvl1_ptrs = {0};
//     std::vector<int> lvl2_ptrs = {0};
//
//     // Generazione procedurale dell'albero CSF asimmetrico
//     for (int i = 0; i < root_size; ++i) {
//         h_all_indices.push_back(i);
//         int num_j_children = (i % 2 == 0) ? 3 : 2;
//         for (int j = 0; j < num_j_children; ++j) {
//             lvl1_ptrs.push_back(0);
//             j_counter++;
//         }
//         h_all_ptrs.push_back(j_counter);
//     }
//
//     int current_j_node = 0;
//     int current_lvl1_idx_start = h_all_indices.size();
//     for (int i = 0; i < root_size; ++i) {
//         int num_j_children = (i % 2 == 0) ? 3 : 2;
//         for (int j = 0; j < num_j_children; ++j) {
//             h_all_indices.push_back(j);
//             int num_k_children = (current_j_node % 2 == 0) ? 2 : 3;
//             for (int k = 0; k < num_k_children; ++k) {
//                 lvl2_ptrs.push_back(0);
//                 k_counter++;
//             }
//             lvl1_ptrs[current_j_node + 1] = k_counter;
//             current_j_node++;
//         }
//     }
//
//     int current_k_node = 0;
//     current_j_node = 0;
//     int current_lvl2_idx_start = h_all_indices.size();
//     for (int i = 0; i < root_size; ++i) {
//         int num_j_children = (i % 2 == 0) ? 3 : 2;
//         for (int j = 0; j < num_j_children; ++j) {
//             int num_k_children = (current_j_node % 2 == 0) ? 2 : 3;
//             for (int k = 0; k < num_k_children; ++k) {
//                 h_all_indices.push_back(k);
//                 current_k_node++;
//             }
//             current_j_node++;
//         }
//     }
//
//     // Input denso di contrazione per 'o'. Dimensione 2
//     std::vector<float> h_dense_in = {2.5f, 4.0f};
//
//     int current_lvl3_idx_start = h_all_indices.size();
//     current_k_node = 0;
//     current_j_node = 0;
//     float mock_val = 1.0f;
//
//     for (int i = 0; i < root_size; ++i) {
//         int num_j_children = (i % 2 == 0) ? 3 : 2;
//         for (int j = 0; j < num_j_children; ++j) {
//             int num_k_children = (current_j_node % 2 == 0) ? 2 : 3;
//             for (int k = 0; k < num_k_children; ++k) {
//                 int num_o_leaves = (k % 2 == 0) ? 2 : 1;
//
//                 float fiber_sum = 0.0f;
//                 for (int o = 0; o < num_o_leaves; ++o) {
//                     h_all_indices.push_back(o);
//                     h_vals.push_back(mock_val);
//
//                     fiber_sum += mock_val * h_dense_in[o];
//                     mock_val += 1.0f;
//                 }
//                 // Il dizionario CPU memorizza la contrazione attesa nello spazio compresso (i,j,k)
//                 cpu_reference_map[std::make_tuple(i, j, k)] = fiber_sum;
//
//                 // Aggiorna i puntatori del livello k per puntare alla fine di questa fibra 'o'
//                 lvl2_ptrs[current_k_node + 1] = h_vals.size();
//                 current_k_node++;
//             }
//             current_j_node++;
//         }
//     }
//
//     UniversalCSF csf{};
//     csf.order = 4;
//     csf.nnz = h_vals.size();
//
//     csf.ptr_offsets[0] = 0;
//     csf.ptr_offsets[1] = h_all_ptrs.size();
//     h_all_ptrs.insert(h_all_ptrs.end(), lvl1_ptrs.begin(), lvl1_ptrs.end());
//
//     csf.ptr_offsets[2] = h_all_ptrs.size();
//     h_all_ptrs.insert(h_all_ptrs.end(), lvl2_ptrs.begin(), lvl2_ptrs.end());
//
//     csf.ind_offsets[0] = 0;
//     csf.ind_offsets[1] = current_lvl1_idx_start;
//     csf.ind_offsets[2] = current_lvl2_idx_start;
//     csf.ind_offsets[3] = current_lvl3_idx_start;
//
//     std::cout << " -> Struttura CSF generata. Foglie iniziali (NNZ): " << csf.nnz << std::endl;
//     std::cout << " -> Nodi attesi nello spazio contratto 3D (CPU Map size): " << cpu_reference_map.size() << std::endl;
//
//     // Copia delle strutture su GPU
//     CUDA_CHECK(cudaMalloc(&csf.d_all_ptrs, h_all_ptrs.size() * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&csf.d_all_indices, h_all_indices.size() * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
//
//     CUDA_CHECK(cudaMemcpy(csf.d_all_ptrs, h_all_ptrs.data(), h_all_ptrs.size() * sizeof(int), cudaMemcpyHostToDevice));
//     CUDA_CHECK(cudaMemcpy(csf.d_all_indices, h_all_indices.data(), h_all_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
//     CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
//
//     float* d_dense_in;
//     CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
//     CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));
//
//     // Allocazione del buffer di output COO contratto (Rango finale = csf.order - plan.num_contracted_modes = 3)
//     int out_order = csf.order - plan.num_contracted_modes;
//     int max_expected_out_nnz = cpu_reference_map.size();
//
//     int* d_out_indices;
//     float* d_out_vals;
//     int* d_global_counter;
//     CUDA_CHECK(cudaMalloc(&d_out_indices, max_expected_out_nnz * out_order * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&d_out_vals, max_expected_out_nnz * sizeof(float)));
//     CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
//     CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));
//
//     // Invocazione Kernel CUDA
//     launch_csf_einsum(csf, plan, d_dense_in, root_size, d_out_indices, d_out_vals, d_global_counter);
//     CUDA_CHECK(cudaDeviceSynchronize());
//
//     // Recupero del numero esatto di elementi generati
//     int h_final_nnz = 0;
//     CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
//
//     std::cout << " -> Calcolo GPU Terminato. Elementi validi in uscita: " << h_final_nnz << std::endl;
//
//     // VALIDAZIONE COERENTE: Il numero di percorsi contratti unici deve coincidere con la mappa della CPU
//     assert(h_final_nnz == max_expected_out_nnz && "Errore: Discrepanza nel conteggio dei nodi finali compressi.");
//
//     std::vector<int> h_out_indices(h_final_nnz * out_order);
//     std::vector<float> h_out_vals(h_final_nnz);
//     CUDA_CHECK(cudaMemcpy(h_out_indices.data(), d_out_indices, h_final_nnz * out_order * sizeof(int), cudaMemcpyDeviceToHost));
//     CUDA_CHECK(cudaMemcpy(h_out_vals.data(), d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));
//
//     bool global_success = true;
//     for (int n = 0; n < h_final_nnz; ++n) {
//         int i = h_out_indices[n * out_order + 0];
//         int j = h_out_indices[n * out_order + 1];
//         int k = h_out_indices[n * out_order + 2];
//         float val_ottenuto = h_out_vals[n];
//
//         Coord3D c = std::make_tuple(i, j, k);
//         if (cpu_reference_map.find(c) == cpu_reference_map.end()) {
//             std::cout << "[ERRORE] La GPU ha calcolato una coordinata inesistente nello spazio ridotto: (" << i << "," << j << "," << k << ")" << std::endl;
//             global_success = false;
//             break;
//         }
//
//         float val_atteso = cpu_reference_map[c];
//
//         if (std::abs(val_ottenuto - val_atteso) > 1e-3f) {
//             std::cout << "[ERRORE] Discrepanza numerica alla coordinata (" << i << "," << j << "," << k
//                       << ") -> Ricevuto: " << val_ottenuto << ", Atteso: " << val_atteso << std::endl;
//             global_success = false;
//         }
//     }
//
//     if (global_success) {
//         std::cout << "\n[STRESS TEST SUPERATO] Tutti i percorsi sono stati contratti e validati correttamente senza duplicati." << std::endl;
//     } else {
//         std::cout << "\n[FALLITO] Rilevato un errore di calcolo o strutturale." << std::endl;
//         assert(false);
//     }
//
//     cudaFree(csf.d_all_ptrs); cudaFree(csf.d_all_indices); cudaFree(csf.d_vals);
//     cudaFree(d_dense_in); cudaFree(d_out_indices); cudaFree(d_out_vals); cudaFree(d_global_counter);
//     return 0;
// #endif
//     std::cout << "[STRESS TEST CORRETTO] Esecuzione parallela Leaf-Centric..." << std::endl;
//
//     std::string expression = "ijko,o->ijk";
//     EinsumPlan plan = compile_einsum_string_simple(expression);
//
//     // Mappa CPU per raccogliere i dati validati
//     typedef std::tuple<int, int, int> Coord3D;
//     std::map<Coord3D, float> cpu_reference_map;
//
//     std::vector<int> h_all_ptrs;
//     std::vector<int> h_all_indices;
//     std::vector<float> h_vals;
//
//     // Generazione di un albero pulito e deterministico per testare il parallelismo di massa
//     // Creiamo esattamente 24 combinazioni (i, j, k) distinte per assicurarci che la GPU veda tutto.
//     int idx_counter = 0;
//     std::vector<int> lvl0_ptrs = {0};
//     std::vector<int> lvl1_ptrs = {0};
//     std::vector<int> lvl2_ptrs = {0};
//
//     std::vector<float> h_dense_in = {2.5f, 4.0f};
//
//     int current_j_global = 0;
//     int current_k_global = 0;
//
//     // Livello 0
//     for(int i=0; i<4; ++i) {
//         h_all_indices.push_back(i);
//         for(int j=0; j<2; ++j) {
//             current_j_global++;
//         }
//         lvl0_ptrs.push_back(current_j_global);
//     }
//
//     int lvl1_start = h_all_indices.size();
//     // Livello 1
//     for(int j_node=0; j_node<current_j_global; ++j_node) {
//         h_all_indices.push_back(j_node % 2);
//         for(int k=0; k<3; ++k) {
//             current_k_global++;
//         }
//         lvl1_ptrs.push_back(current_k_global);
//     }
//
//     int lvl2_start = h_all_indices.size();
//     // Livello 2
//     for(int k_node=0; k_node<current_k_global; ++k_node) {
//         h_all_indices.push_back(k_node % 3);
//         lvl2_ptrs.push_back(0); // Verrà riempito dopo col numero di foglie
//     }
//
//     int lvl3_start = h_all_indices.size();
//     int leaf_val_counter = 1;
//     int num_final_fibers = current_k_global; // Il numero di nodi k (penultimo livello)
//
//     // Livello 3 (Foglie)
//     int total_leaves_collected = 0;
//     lvl2_ptrs[0] = 0;
//
//     int k_idx = 0;
//     for(int i=0; i<4; ++i) {
//         for(int j=0; j<2; ++j) {
//             for(int k=0; k<3; ++k) {
//                 float fiber_sum = 0.0f;
//                 // Ogni fibra ha 2 foglie 'o'
//                 for(int o=0; o<2; ++o) {
//                     h_all_indices.push_back(o);
//                     h_vals.push_back((float)leaf_val_counter);
//
//                     fiber_sum += (float)leaf_val_counter * h_dense_in[o];
//                     leaf_val_counter++;
//                     total_leaves_collected++;
//                 }
//                 cpu_reference_map[std::make_tuple(i, j, k)] = fiber_sum;
//                 lvl2_ptrs[k_idx + 1] = total_leaves_collected;
//                 k_idx++;
//             }
//         }
//     }
//
//     UniversalCSF csf{};
//     csf.order = 4;
//     csf.nnz = h_vals.size();
//
//     // Costruzione degli offset globali di segmentazione
//     csf.ptr_offsets[0] = 0;
//     h_all_ptrs.insert(h_all_ptrs.end(), lvl0_ptrs.begin(), lvl0_ptrs.end());
//
//     csf.ptr_offsets[1] = h_all_ptrs.size();
//     h_all_ptrs.insert(h_all_ptrs.end(), lvl1_ptrs.begin(), lvl1_ptrs.end());
//
//     csf.ptr_offsets[2] = h_all_ptrs.size();
//     h_all_ptrs.insert(h_all_ptrs.end(), lvl2_ptrs.begin(), lvl2_ptrs.end());
//
//     csf.ind_offsets[0] = 0;
//     csf.ind_offsets[1] = lvl1_start;
//     csf.ind_offsets[2] = lvl2_start;
//     csf.ind_offsets[3] = lvl3_start;
//
//     std::cout << " -> Nodi finali dello spazio contratto attesi (CPU): " << cpu_reference_map.size() << std::endl;
//     std::cout << " -> Numero di fibre finali da lanciare in parallelo su GPU: " << num_final_fibers << std::endl;
//
//     // Allocazione GPU
//     CUDA_CHECK(cudaMalloc(&csf.d_all_ptrs, h_all_ptrs.size() * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&csf.d_all_indices, h_all_indices.size() * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
//
//     CUDA_CHECK(cudaMemcpy(csf.d_all_ptrs, h_all_ptrs.data(), h_all_ptrs.size() * sizeof(int), cudaMemcpyHostToDevice));
//     CUDA_CHECK(cudaMemcpy(csf.d_all_indices, h_all_indices.data(), h_all_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
//     CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
//
//     float* d_dense_in;
//     CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
//     CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));
//
//     int out_order = csf.order - plan.num_contracted_modes;
//     int max_expected_out_nnz = cpu_reference_map.size();
//
//     int* d_out_indices; float* d_out_vals; int* d_global_counter;
//     CUDA_CHECK(cudaMalloc(&d_out_indices, max_expected_out_nnz * out_order * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&d_out_vals, max_expected_out_nnz * sizeof(float)));
//     CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
//     CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));
//
//     // INVOCAZIONE PASSANDO IL NUMERO DI FIBRE FINALI (k_nodes) COME CONTESTO DI MULTIPROCESSING
//     // All'interno di main.cpp, la chiamata diventa pulita e diretta:
//     launch_csf_einsum(csf, plan, d_dense_in, csf.nnz, d_out_indices, d_out_vals, d_global_counter);
//     CUDA_CHECK(cudaDeviceSynchronize());
//
//     int h_final_nnz = 0;
//     CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
//     std::cout << " -> Calcolo GPU Terminato. Elementi validi estratti: " << h_final_nnz << std::endl;
//
//     // ORA L'ASSERT PASSERÀ PERFETTAMENTE: 24 == 24
//     assert(h_final_nnz == max_expected_out_nnz && "Errore: Il parallelismo GPU ha saltato dei nodi strutturali.");
//
//     std::vector<int> h_out_indices(h_final_nnz * out_order);
//     std::vector<float> h_out_vals(h_final_nnz);
//     CUDA_CHECK(cudaMemcpy(h_out_indices.data(), d_out_indices, h_final_nnz * out_order * sizeof(int), cudaMemcpyDeviceToHost));
//     CUDA_CHECK(cudaMemcpy(h_out_vals.data(), d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));
//
//     bool global_success = true;
//     for (int n = 0; n < h_final_nnz; ++n) {
//         int i = h_out_indices[n * out_order + 0];
//         int j = h_out_indices[n * out_order + 1];
//         int k = h_out_indices[n * out_order + 2];
//         float val_ottenuto = h_out_vals[n];
//
//         Coord3D c = std::make_tuple(i, j, k);
//         float val_atteso = cpu_reference_map[c];
//
//         if (std::abs(val_ottenuto - val_atteso) > 1e-3f) {
//             std::cout << "[ERRORE] Discrepanza alla coordinata (" << i << "," << j << "," << k
//                       << ") -> Ricevuto: " << val_ottenuto << ", Atteso: " << val_atteso << std::endl;
//             global_success = false;
//         }
//     }
//
//     if (global_success) {
//         std::cout << "\n[STRESS TEST 100% SUCCESS] La GPU calcola tutti i cammini senza alcuna perdita strutturale!" << std::endl;
//     }
//
//     cudaFree(csf.d_all_ptrs); cudaFree(csf.d_all_indices); cudaFree(csf.d_vals);
//     cudaFree(d_dense_in); cudaFree(d_out_indices); cudaFree(d_out_vals); cudaFree(d_global_counter);
//     return 0;
// }
//
// // int main() {
// //     std::cout << "[STRESS TEST] Generazione di una struttura CSF ad alto volume..." << std::endl;
// //
// //     // 1. Compilazione del piano Einsum "ijko,o->ijk"
// //     std::string expression = "ijko,o->ijk";
// //     EinsumPlan plan = compile_einsum_string_simple(expression);
// //
// //     // 2. Definizione di un albero CSF 4D sintetico controllato (Dimensione logica 4x3x3x2)
// //     // Generiamo una struttura asimmetrica densa ma codificata esplicitamente in CSF
// //     // Livello 0 (i) ha 4 nodi radice.
// //     int root_size = 4;
// //
// //     // Costruiamo i vettori strutturali dell'albero CSF linearizzato
// //     std::vector<int> h_all_ptrs;
// //     std::vector<int> h_all_indices;
// //     std::vector<float> h_vals;
// //
// //     // Per tracciare la validazione CPU (Coordinate complete -> Valore originale)
// //     typedef std::tuple<int, int, int, int> Coord4D;
// //     std::map<Coord4D, float> cpu_reference_map;
// //
// //     // --- COSTRUZIONE MANUALE PROCEDURALE DI UN ALBERO VALIDO ---
// //     // Livello 0 (i)
// //     h_all_ptrs.push_back(0); // Inizio livello 0
// //     int j_counter = 0;
// //     int k_counter = 0;
// //     int o_counter = 0;
// //
// //     std::vector<int> lvl1_ptrs = {0};
// //     std::vector<int> lvl2_ptrs = {0};
// //
// //     for (int i = 0; i < root_size; ++i) {
// //         h_all_indices.push_back(i); // Indice della radice i
// //
// //         // Ogni radice 'i' ha un numero variabile di figli 'j' (Asimmetria)
// //         int num_j_children = (i % 2 == 0) ? 3 : 2;
// //         for (int j = 0; j < num_j_children; ++j) {
// //             lvl1_ptrs.push_back(0); // Placeholder che sistemeremo dopo
// //             j_counter++;
// //         }
// //         h_all_ptrs.push_back(j_counter);
// //     }
// //
// //     // Livello 1 (j)
// //     int current_lvl1_idx_start = h_all_indices.size();
// //     int current_j_node = 0;
// //     for (int i = 0; i < root_size; ++i) {
// //         int num_j_children = (i % 2 == 0) ? 3 : 2;
// //         for (int j = 0; j < num_j_children; ++j) {
// //             h_all_indices.push_back(j); // Indice strutturale 'j'
// //
// //             // Ogni nodo 'j' si ramifica in figli 'k'
// //             int num_k_children = (current_j_node % 2 == 0) ? 2 : 3;
// //             for (int k = 0; k < num_k_children; ++k) {
// //                 lvl2_ptrs.push_back(0);
// //                 k_counter++;
// //             }
// //             lvl1_ptrs[current_j_node + 1] = k_counter;
// //             current_j_node++;
// //         }
// //     }
// //
// //     // Livello 2 (k)
// //     int current_lvl2_idx_start = h_all_indices.size();
// //     int current_k_node = 0;
// //     current_j_node = 0;
// //     for (int i = 0; i < root_size; ++i) {
// //         int num_j_children = (i % 2 == 0) ? 3 : 2;
// //         for (int j = 0; j < num_j_children; ++j) {
// //             int num_k_children = (current_j_node % 2 == 0) ? 2 : 3;
// //             for (int k = 0; k < num_k_children; ++k) {
// //                 h_all_indices.push_back(k); // Indice strutturale 'k'
// //
// //                 // Ogni nodo 'k' produce da 1 a 2 foglie sull'asse 'o'
// //                 int num_o_leaves = (k % 2 == 0) ? 2 : 1;
// //                 for (int o = 0; o < num_o_leaves; ++o) {
// //                     o_counter++;
// //                 }
// //                 lvl2_ptrs[current_k_node + 1] = o_counter;
// //                 current_k_node++;
// //             }
// //             current_j_node++;
// //         }
// //     }
// //
// //     // Livello 3 (o) e Valori Foglia
// //     int current_lvl3_idx_start = h_all_indices.size();
// //     current_k_node = 0;
// //     current_j_node = 0;
// //     float mock_val = 1.0f;
// //
// //     for (int i = 0; i < root_size; ++i) {
// //         int num_j_children = (i % 2 == 0) ? 3 : 2;
// //         for (int j = 0; j < num_j_children; ++j) {
// //             int num_k_children = (current_j_node % 2 == 0) ? 2 : 3;
// //             for (int k = 0; k < num_k_children; ++k) {
// //                 int num_o_leaves = (k % 2 == 0) ? 2 : 1;
// //                 for (int o = 0; o < num_o_leaves; ++o) {
// //                     h_all_indices.push_back(o); // Indice finale 'o'
// //                     h_vals.push_back(mock_val);
// //
// //                     // Salviamo il cammino esplicito per la validazione CPU
// //                     cpu_reference_map[std::make_tuple(i, j, k, o)] = mock_val;
// //                     mock_val += 1.0f;
// //                 }
// //                 current_k_node++;
// //             }
// //             current_j_node++;
// //         }
// //     }
// //
// //     // Concateniamo i vettori dei puntatori temporanei dentro h_all_ptrs complessivo
// //     UniversalCSF csf{};
// //     csf.order = 4;
// //     csf.nnz = h_vals.size();
// //
// //     csf.ptr_offsets[0] = 0;
// //     csf.ptr_offsets[1] = h_all_ptrs.size();
// //     h_all_ptrs.insert(h_all_ptrs.end(), lvl1_ptrs.begin(), lvl1_ptrs.end());
// //
// //     csf.ptr_offsets[2] = h_all_ptrs.size();
// //     h_all_ptrs.insert(h_all_ptrs.end(), lvl2_ptrs.begin(), lvl2_ptrs.end());
// //
// //     csf.ind_offsets[0] = 0;
// //     csf.ind_offsets[1] = current_lvl1_idx_start;
// //     csf.ind_offsets[2] = current_lvl2_idx_start;
// //     csf.ind_offsets[3] = current_lvl3_idx_start;
// //
// //     std::cout << " -> Struttura generata con successo. NNZ totali (foglie): " << csf.nnz << std::endl;
// //
// //     // Vettore denso di contrazione per 'o'. Dimensione 2
// //     std::vector<float> h_dense_in = {2.5f, 4.0f};
// //
// //     // Copia delle strutture sul Device
// //     CUDA_CHECK(cudaMalloc(&csf.d_all_ptrs, h_all_ptrs.size() * sizeof(int)));
// //     CUDA_CHECK(cudaMalloc(&csf.d_all_indices, h_all_indices.size() * sizeof(int)));
// //     CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
// //
// //     CUDA_CHECK(cudaMemcpy(csf.d_all_ptrs, h_all_ptrs.data(), h_all_ptrs.size() * sizeof(int), cudaMemcpyHostToDevice));
// //     CUDA_CHECK(cudaMemcpy(csf.d_all_indices, h_all_indices.data(), h_all_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
// //     CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
// //
// //     float* d_dense_in;
// //     CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
// //     CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));
// //
// //     // 3. Allocazione del buffer di output COO
// //     int* d_out_indices;
// //     float* d_out_vals;
// //     int* d_global_counter;
// //     CUDA_CHECK(cudaMalloc(&d_out_indices, csf.nnz * csf.order * sizeof(int)));
// //     CUDA_CHECK(cudaMalloc(&d_out_vals, csf.nnz * sizeof(float)));
// //     CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
// //     CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));
// //
// //     // 4. Invocazione Kernel CUDA
// //     launch_csf_einsum(csf, plan, d_dense_in, root_size, d_out_indices, d_out_vals, d_global_counter);
// //     CUDA_CHECK(cudaDeviceSynchronize());
// //
// //     // 5. Recupero e Validazione Totale dei risultati
// //     int h_final_nnz = 0;
// //     CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
// //
// //     assert(h_final_nnz == csf.nnz && "Errore fatale: il numero di nodi processati non corrisponde alle foglie totali.");
// //
// //     std::vector<int> h_out_indices(h_final_nnz * csf.order);
// //     std::vector<float> h_out_vals(h_final_nnz);
// //     CUDA_CHECK(cudaMemcpy(h_out_indices.data(), d_out_indices, h_final_nnz * csf.order * sizeof(int), cudaMemcpyDeviceToHost));
// //     CUDA_CHECK(cudaMemcpy(h_out_vals.data(), d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));
// //
// //     bool global_success = true;
// //     for (int n = 0; n < h_final_nnz; ++n) {
// //         int i = h_out_indices[n * csf.order + 0];
// //         int j = h_out_indices[n * csf.order + 1];
// //         int k = h_out_indices[n * csf.order + 2];
// //         int o = h_out_indices[n * csf.order + 3];
// //         float val_ottenuto = h_out_vals[n];
// //
// //         // Controllo incrociato sul dizionario CPU di riferimento
// //         Coord4D c = std::make_tuple(i, j, k, o);
// //         if (cpu_reference_map.find(c) == cpu_reference_map.end()) {
// //             std::cout << "[ERRORE] La GPU ha calcolato una coordinata inesistente: (" << i << "," << j << "," << k << "," << o << ")" << std::endl;
// //             global_success = false;
// //             break;
// //         }
// //
// //         float val_originale = cpu_reference_map[c];
// //         float val_atteso = val_originale * h_dense_in[o];
// //
// //         if (std::abs(val_ottenuto - val_atteso) > 1e-4f) {
// //             std::cout << "[ERRORE] Discrepanza sul valore alla coordinata (" << i << "," << j << "," << k << "," << o
// //                       << ") -> Ricevuto: " << val_ottenuto << ", Atteso: " << val_atteso << std::endl;
// //             global_success = false;
// //         }
// //     }
// //
// //     if (global_success) {
// //         std::cout << "\n[STRESS TEST SUPERATO] Verificati con successo tutti i " << h_final_nnz
// //         << " cammini complessi dell'albero CSF." << std::endl;
// //     } else {
// //         std::cout << "\n[FALLITO] Rilevati errori strutturali o numerici." << std::endl;
// //         assert(false);
// //     }
// //     // Pulizia
// //     cudaFree(csf.d_all_ptrs); cudaFree(csf.d_all_indices); cudaFree(csf.d_vals);
// //     cudaFree(d_dense_in); cudaFree(d_out_indices); cudaFree(d_out_vals); cudaFree(d_global_counter);
// //     return 0;
// // }
//
// //
// // int main() {
// //     std::cout << "[TEST] Avvio del test di verifica per Einsum CSF ad ordine arbitrario..." << std::endl;
// //
// //     // ==========================================================
// //     // 1. COMPILAZIONE DELL'ESPRESSIONE TRAMITE PRIMITIVA
// //     // ==========================================================
// //     std::string expression = "ijko,o->ijk";
// //     EinsumPlan plan = compile_einsum_string_simple(expression);
// //
// //     // ==========================================================
// //     // 2. CONFIGURAZIONE DEI DATI DI INPUT (CSF E VETTORE DENSO)
// //     // ==========================================================
// //     // UniversalCSF csf{};
// //     // csf.order = 4;
// //     // csf.nnz = 4;
// //     //
// //     // // Struttura fissa di un albero CSF linearizzato 4D per il test
// //     // // Rappresenta strutturalmente 4 nodi foglia validi
// //     // std::vector<int> h_all_ptrs = {0, 2, 4, 0, 1, 3, 5, 8};
// //     // std::vector<int> h_all_indices = {0, 1, 0, 1, 2, 0, 1, 0, 1, 0, 1};
// //     //
// //     // // Assegniamo valori sparse ben definiti per il test
// //     // std::vector<float> h_vals = {10.0f, 20.0f, 30.0f, 40.0f};
// //     //
// //     // // Impostazione degli array statici di offset
// //     // for(int i = 0; i < MAX_TENSOR_ORDER; ++i) {
// //     //     csf.ptr_offsets[i] = 0;
// //     //     csf.ind_offsets[i] = 0;
// //     // }
// //     // csf.ptr_offsets[0] = 0; csf.ptr_offsets[1] = 3;
// //     // csf.ind_offsets[0] = 0; csf.ind_offsets[1] = 2; csf.ind_offsets[2] = 5; csf.ind_offsets[3] = 8;
// //     UniversalCSF csf{};
// //     csf.order = 4;
// //     csf.nnz = 2; // Testiamo con 2 elementi non zeri reali nel fogliame
// //
// //     // Struttura CSF 4D valida:
// //     // Livello 0 (i): 2 nodi radice -> puntano alle posizioni [0,1) e [1,2) del Livello 1
// //     // Livello 1 (j): 2 nodi -> puntano alle posizioni [0,1) e [1,2) del Livello 2
// //     // Livello 2 (k): 2 nodi -> puntano alle posizioni [0,1) e [1,2) del Livello 3 (Foglie)
// //     // Livello 3 (o): 2 nodi finali (Valori)
// //     std::vector<int> h_all_ptrs = {
// //         0, 1, 2, // Livello 0: ptr_offset = 0 (Dimensione: root_size + 1 = 3)
// //         0, 1, 2, // Livello 1: ptr_offset = 3
// //         0, 1, 2, // Livello 2: ptr_offset = 6
// //         0, 1, 2  // Livello 3: ptr_offset = 9
// //     };
// //
// //     std::vector<int> h_all_indices = {
// //         0, 1, // Livello 0: indici 'i' -> ind_offset = 0
// //         0, 1, // Livello 1: indici 'j' -> ind_offset = 2
// //         0, 1, // Livello 2: indici 'k' -> ind_offset = 4
// //         0, 1  // Livello 3: indici 'o' -> ind_offset = 6
// //     };
// //
// //     // Valori associati alle foglie
// //     std::vector<float> h_vals = {10.0f, 20.0f};
// //
// //     // Inizializzazione pulita a zero degli array statici degli offset
// //     for(int i = 0; i < MAX_TENSOR_ORDER; ++i) {
// //         csf.ptr_offsets[i] = 0;
// //         csf.ind_offsets[i] = 0;
// //     }
// //
// //     // Mappatura degli offset statici per navigare i vettori linearizzati sopra descritti
// //     csf.ptr_offsets[0] = 0;
// //     csf.ptr_offsets[1] = 3;
// //     csf.ptr_offsets[2] = 6;
// //     csf.ptr_offsets[3] = 9;
// //
// //     csf.ind_offsets[0] = 0;
// //     csf.ind_offsets[1] = 2;
// //     csf.ind_offsets[2] = 4;
// //     csf.ind_offsets[3] = 6;
// //
// //     // Allocazione e copia dei dati del Tensore CSF sulla GPU
// //     CUDA_CHECK(cudaMalloc(&csf.d_all_ptrs, h_all_ptrs.size() * sizeof(int)));
// //     CUDA_CHECK(cudaMalloc(&csf.d_all_indices, h_all_indices.size() * sizeof(int)));
// //     CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
// //
// //     CUDA_CHECK(cudaMemcpy(csf.d_all_ptrs, h_all_ptrs.data(), h_all_ptrs.size() * sizeof(int), cudaMemcpyHostToDevice));
// //     CUDA_CHECK(cudaMemcpy(csf.d_all_indices, h_all_indices.data(), h_all_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
// //     CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
// //
// //     // Input denso (Vettore associato alla coordinata 'o', con dimensione 2)
// //     // Se la coordinata foglia 'o' è 0 moltiplica per 2.0, se è 1 moltiplica per 5.0
// //     std::vector<float> h_dense_in = {2.0f, 5.0f};
// //     float* d_dense_in;
// //     CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
// //     CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));
// //
// //     // ==========================================================
// //     // 3. ALLOCAZIONE DEI BUFFER DI OUTPUT (COO INTERMEDIO/FINALE)
// //     // ==========================================================
// //     int* d_out_indices;
// //     float* d_out_vals;
// //     int* d_global_counter;
// //
// //     CUDA_CHECK(cudaMalloc(&d_out_indices, csf.nnz * csf.order * sizeof(int)));
// //     CUDA_CHECK(cudaMalloc(&d_out_vals, csf.nnz * sizeof(float)));
// //     CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
// //     CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));
// //
// //     // ==========================================================
// //     // 4. INVOCAZIONE E CONTROLLO DEI RISULTATI
// //     // ==========================================================
// //     int root_size = 2; // Numero di nodi radice al livello 0
// //     launch_csf_einsum(csf, plan, d_dense_in, root_size, d_out_indices, d_out_vals, d_global_counter);
// //     CUDA_CHECK(cudaDeviceSynchronize());
// //
// //     // Recupero del numero totale di non-zeri (NNZ) generati dall'output COO
// //     int h_final_nnz = 0;
// //     CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
// //
// //     // Recupero dei buffer COO dalla GPU all'Host
// //     std::vector<int> h_out_indices(h_final_nnz * csf.order);
// //     std::vector<float> h_out_vals(h_final_nnz);
// //     CUDA_CHECK(cudaMemcpy(h_out_indices.data(), d_out_indices, h_final_nnz * csf.order * sizeof(int), cudaMemcpyDeviceToHost));
// //     CUDA_CHECK(cudaMemcpy(h_out_vals.data(), d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));
// //
// //     // ==========================================================
// //     // 5. VALIDAZIONE FORMALE AUTOMATICA (ASSERT)
// //     // ==========================================================
// //     std::cout << "\n--- Verifica Elementi Generati ---" << std::endl;
// //     bool test_passed = true;
// //
// //     for (int n = 0; n < h_final_nnz; ++n) {
// //         int i = h_out_indices[n * csf.order + 0];
// //         int j = h_out_indices[n * csf.order + 1];
// //         int k = h_out_indices[n * csf.order + 2];
// //         int o = h_out_indices[n * csf.order + 3]; // Coordinata contratta preservata nel COO intermedio
// //         float val_ottenuto = h_out_vals[n];
// //
// //         // Calcolo del valore atteso teorico
// //         // Recuperiamo il valore sparse originale basandoci sull'indice dell'elemento nel test
// //         float val_originale = h_vals[n];
// //         float moltiplicatore_denso = h_dense_in[o];
// //         float val_atteso = val_originale * moltiplicatore_denso;
// //
// //         std::cout << "COO [" << n << "] -> Coordinata: (" << i << "," << j << "," << k << "," << o
// //                   << ") | Calcolato: " << val_ottenuto << " | Atteso: " << val_atteso << std::endl;
// //
// //         // Controllo numerico di precisione
// //         if (std::abs(val_ottenuto - val_atteso) > 1e-5f) {
// //             test_passed = false;
// //         }
// //     }
// //
// //     if (test_passed && h_final_nnz > 0) {
// //         std::cout << "\n[TEST SUPERATO SUCCESS] Tutti i valori intermedi COO combaciano al 100%." << std::endl;
// //     } else {
// //         std::cout << "\n[TEST FALLITO ERROR] Discrepanza rilevata nei calcoli GPU o nessun elemento generato." << std::endl;
// //         assert(false);
// //     }
// //
// //     // Pulizia risorse GPU
// //     cudaFree(csf.d_all_ptrs); cudaFree(csf.d_all_indices); cudaFree(csf.d_vals);
// //     cudaFree(d_dense_in); cudaFree(d_out_indices); cudaFree(d_out_vals); cudaFree(d_global_counter);
// //
// //     return 0;
// // }
// //
// // //
// // // int main() {
// // //     // ==========================================
// // //     // 1. CONFIGURAZIONE STATICA DEL PIANO EINSUM
// // //     // ==========================================
// // //     // Esempio: Contrazione di un tensore 4D su asse 3 ("ijko,o->ijk")
// // //     // EinsumPlan plan{};
// // //     // plan.tensor_order = 4;
// // //     // plan.num_contracted_modes = 1;
// // //     //
// // //     // // Inizializzazione degli array statici dell'host
// // //     // for(int i = 0; i < MAX_TENSOR_ORDER; ++i) {
// // //     //     plan.contracted_levels[i] = 0;
// // //     //     plan.dense_dim_strides[i] = 0;
// // //     // }
// // //     // plan.contracted_levels[0] = 3;
// // //     // plan.dense_dim_strides[0] = 1;
// // //     // Compiliamo la stringa classica
// // //     std::string einsum_expr = "ijko,o->ijk";
// // //     EinsumPlan plan = compile_einsum_string_simple(einsum_expr);
// // //
// // //     // Stampa di controllo per verificare la compilazione statica
// // //     std::cout << "--- Piano Einsum Compilato ---" << std::endl;
// // //     std::cout << "Rango Tensore Sparse: " << plan.tensor_order << std::endl;
// // //     std::cout << "Assi Contratti totali: " << plan.num_contracted_modes << std::endl;
// // //     for(int i = 0; i < plan.num_contracted_modes; ++i) {
// // //         std::cout << " -> Livello CSF contratto: " << plan.contracted_levels[i]
// // //                   << " con Stride denso = " << plan.dense_dim_strides[i] << std::endl;
// // //     }
// // //
// // //     // ==========================================
// // //     // 2. PREPARAZIONE E MOCK DATA IN INGRESSO (CSF)
// // //     // ==========================================
// // //     UniversalCSF csf{};
// // //     csf.order = 4;
// // //     csf.nnz = 8;
// // //
// // //     // Rappresentazione lineare concatenata di un albero CSF fittizio
// // //     std::vector<int> h_all_ptrs = {0, 2, 4, 0, 1, 3, 5, 8};
// // //     std::vector<int> h_all_indices = {0, 1, 0, 1, 2, 0, 1, 0, 1, 0, 1};
// // //     std::vector<float> h_vals(csf.nnz, 2.5f);
// // //
// // //     // Configurazione degli array di offset statici
// // //     csf.ptr_offsets[0] = 0; csf.ptr_offsets[1] = 3; csf.ptr_offsets[2] = 0; csf.ptr_offsets[3] = 0;
// // //     csf.ind_offsets[0] = 0; csf.ind_offsets[1] = 2; csf.ind_offsets[2] = 5; csf.ind_offsets[3] = 8;
// // //
// // //     // Allocazione ed invio delle strutture Dati sul Device
// // //     CUDA_CHECK(cudaMalloc(&csf.d_all_ptrs, h_all_ptrs.size() * sizeof(int)));
// // //     CUDA_CHECK(cudaMalloc(&csf.d_all_indices, h_all_indices.size() * sizeof(int)));
// // //     CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
// // //
// // //     CUDA_CHECK(cudaMemcpy(csf.d_all_ptrs, h_all_ptrs.data(), h_all_ptrs.size() * sizeof(int), cudaMemcpyHostToDevice));
// // //     CUDA_CHECK(cudaMemcpy(csf.d_all_indices, h_all_indices.data(), h_all_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
// // //     CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
// // //
// // //     // Input denso (Vettore di dimensione 2)
// // //     std::vector<float> h_dense_in = {2.0f, 2.0f};
// // //     float* d_dense_in;
// // //     CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
// // //     CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));
// // //
// // //     // ==========================================
// // //     // 3. ALLOCAZIONE DEI BUFFER COO DI OUTPUT
// // //     // ==========================================
// // //     int* d_out_indices;
// // //     float* d_out_vals;
// // //     int* d_global_counter;
// // //
// // //     // Lo spazio allocato supporta nel caso peggiore l'intera dimensione dei nodi foglia
// // //     CUDA_CHECK(cudaMalloc(&d_out_indices, csf.nnz * csf.order * sizeof(int)));
// // //     CUDA_CHECK(cudaMalloc(&d_out_vals, csf.nnz * sizeof(float)));
// // //     CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
// // //     CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));
// // //
// // //     // ==========================================
// // //     // 4. ESECUZIONE
// // //     // ==========================================
// // //     int root_size = 2; // Numero di entry point al livello Radice (0)
// // //
// // //     launch_csf_einsum(csf, plan, d_dense_in, root_size, d_out_indices, d_out_vals, d_global_counter);
// // //     CUDA_CHECK(cudaDeviceSynchronize());
// // //
// // //     // Verifica dei risultati estraendo il contatore COO generato
// // //     int h_final_nnz = 0;
// // //     CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
// // //
// // //     std::cout << "[SUCCESS] Einsum calcolato con successo utilizzando array statici.\n";
// // //     std::cout << "Elementi validi scritti nel buffer COO di output: " << h_final_nnz << std::endl;
// // //
// // //     // Liberazione memoria GPU
// // //     cudaFree(csf.d_all_ptrs);
// // //     cudaFree(csf.d_all_indices);
// // //     cudaFree(csf.d_vals);
// // //     cudaFree(d_dense_in);
// // //     cudaFree(d_out_indices);
// // //     cudaFree(d_out_vals);
// // //     cudaFree(d_global_counter);
// // //
// // //     return 0;
// // // }
