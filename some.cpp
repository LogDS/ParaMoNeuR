#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cassert>
#include <map>
#include <stdexcept>
#include "csf_einsum.cuh"

#define CUDA_CHECK(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true) {
   if (code != cudaSuccess) {
      fprintf(stderr, "GPU Error: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}


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

//
// // Compilatore Host reale: estrae strides e coordinate contratte da QUALSIASI stringa compatibile
// EinsumPlan compile_einsum_string(const std::string& expression, const int* dense_sizes) {
//     EinsumPlan plan{};
//     plan.tensor_order = 0;
//     plan.num_contracted_modes = 0;
//     for (int i = 0; i < MAX_TENSOR_ORDER; ++i) {
//         plan.contracted_levels[i] = -1;
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
//     std::string dense_labels  = expression.substr(comma_pos + 1, arrow_pos - comma_pos - 1);
//     std::string output_labels = expression.substr(arrow_pos + 2);
//
//     plan.tensor_order = static_cast<int>(sparse_labels.length());
//
//     for (int i = 0; i < plan.tensor_order; ++i) {
//         char current_label = sparse_labels[i];
//         if (output_labels.find(current_label) == std::string::npos) {
//             plan.contracted_levels[plan.num_contracted_modes] = i;
//
//             int stride = 1;
//             size_t dense_pos = dense_labels.find(current_label);
//             if (dense_pos != std::string::npos) {
//                 for (size_t d = dense_labels.length() - 1; d > dense_pos; --d) {
//                     stride *= dense_sizes[d];
//                 }
//                 plan.dense_dim_strides[plan.num_contracted_modes] = stride;
//             }
//             plan.num_contracted_modes++;
//         }
//     }
//     return plan;
// }
//

#include <algorithm>

// 2. IMPLEMENTAZIONE DEL COSTRUTTORE AUTOMATICO DA COO A TENSOR DEVICE
UniversalCSF build_universal_csf_from_coo(const std::vector<RawCOOElement>& raw_elements, int tensor_order) {
    UniversalCSF csf{};
    csf.order = tensor_order;
    csf.nnz = static_cast<int>(raw_elements.size());

    if (csf.nnz == 0) {
        csf.d_vals = nullptr;
        csf.d_coordinate_matrix = nullptr;
        return csf;
    }

    // Ordinamento lessicografico stabile per le coordinate sparse
    std::vector<RawCOOElement> sorted_elements = raw_elements;
    std::sort(sorted_elements.begin(), sorted_elements.end(), [tensor_order](const RawCOOElement& a, const RawCOOElement& b) {
        for (int d = 0; d < tensor_order; ++d) {
            if (a.coords[d] != b.coords[d]) {
                return a.coords[d] < b.coords[d];
            }
        }
        return false;
    });

    std::vector<float> h_vals(csf.nnz);
    std::vector<int> h_coordinate_matrix;
    h_coordinate_matrix.reserve(csf.nnz * tensor_order);

    for (int n = 0; n < csf.nnz; ++n) {
        h_vals[n] = sorted_elements[n].val;
        for (int d = 0; d < tensor_order; ++d) {
            h_coordinate_matrix.push_back(sorted_elements[n].coords[d]);
        }
    }

    // Allocazione fisica lineare dei segmenti contigui sulla VRAM
    CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&csf.d_coordinate_matrix, h_coordinate_matrix.size() * sizeof(int)));

    CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(csf.d_coordinate_matrix, h_coordinate_matrix.data(), h_coordinate_matrix.size() * sizeof(int), cudaMemcpyHostToDevice));

    return csf;
}

// 3. SUITE DI VERIFICA ED ESECUZIONE
int main() {
    std::cout << "[STRESS TEST FINALE] Inizializzazione pipeline generica end-to-end..." << std::endl;

    // Definiamo un operando denso (es. dimensione asse o = 2)
    int dense_sizes[] = {2};
    std::string expression = "ijko,o->ijk";
    EinsumPlan plan = compile_einsum_string_simple(expression);

    // MOCK DATASET: Vettore di elementi COO totalmente disordinati
    std::vector<RawCOOElement> raw_dataset = {
        {{1, 1, 1, 1}, 16.0f}, {{0, 0, 0, 0}, 1.0f},  {{1, 0, 1, 0}, 11.0f},
        {{0, 0, 0, 1}, 2.0f},  {{0, 0, 1, 0}, 3.0f},  {{0, 0, 1, 1}, 4.0f},
        {{0, 1, 0, 0}, 5.0f},  {{0, 1, 0, 1}, 6.0f},  {{0, 1, 1, 0}, 7.0f},
        {{0, 1, 1, 1}, 8.0f},  {{1, 0, 0, 0}, 9.0f},  {{1, 0, 0, 1}, 10.0f},
        {{1, 0, 1, 1}, 12.0f}, {{1, 1, 0, 0}, 13.0f}, {{1, 1, 0, 1}, 14.0f},
        {{1, 1, 1, 0}, 15.0f}
    };

    // Il costruttore ordina automaticamente il dataset ed effettua l'upload su GPU
    UniversalCSF csf = build_universal_csf_from_coo(raw_dataset, 4);
    std::cout << " -> Dataset caricato con successo. Elementi non-zero (NNZ): " << csf.nnz << std::endl;

    std::vector<float> h_dense_in = {2.0f, 5.0f};
    float* d_dense_in;
    CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));

    // Struttura CPU di riferimento per raccogliere ed accumulare i dati contratti
    std::map<std::vector<int>, float> cpu_map;
    for (const auto& elem : raw_dataset) {
        int i = elem.coords[0]; int j = elem.coords[1]; int k = elem.coords[2]; int o = elem.coords[3];
        cpu_map[{i, j, k}] += elem.val * h_dense_in[o];
    }

    // Allocazione del buffer di output COO contratto (Rango finale = 4 - 1 = 3)
    int out_order = csf.order - plan.num_contracted_modes;
    int* d_out_indices; float* d_out_vals; int* d_global_counter;

    CUDA_CHECK(cudaMalloc(&d_out_indices, csf.nnz * out_order * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out_vals, csf.nnz * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
    CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));

    // Lancio del motore GPU
    launch_csf_einsum(csf, plan, d_dense_in, d_out_indices, d_out_vals, d_global_counter);
    CUDA_CHECK(cudaDeviceSynchronize());

    int h_final_nnz = 0;
    CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
    std::cout << " -> Calcolo GPU terminato. Elementi scritti nel buffer intermedio: " << h_final_nnz << std::endl;

    if (h_final_nnz == 0) {
        std::cout << "[FAIL] Nessun record emesso." << std::endl;
        return -1;
    }

    std::vector<int> h_out_indices(h_final_nnz * out_order);
    std::vector<float> h_out_vals(h_final_nnz);

    // Download protetto dei dati
    CUDA_CHECK(cudaMemcpy(h_out_indices.data(), d_out_indices, h_final_nnz * out_order * sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_out_vals.data(), d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));

    // Aggregazione finale post-contrazione (somma dei record intermedi con la stessa coordinata ridotta)
    std::map<std::vector<int>, float> gpu_aggregated_map;
    for(int n = 0; n < h_final_nnz; ++n) {
        std::vector<int> coord = {h_out_indices[n * out_order], h_out_indices[n * out_order + 1], h_out_indices[n * out_order + 2]};
        gpu_aggregated_map[coord] += h_out_vals[n];
    }

    // Controllo di precisione formale
    bool pass = true;
    for (auto const& [coord, val_cpu] : cpu_map) {
        if (std::abs(gpu_aggregated_map[coord] - val_cpu) > 1e-3f) {
            pass = false;
            std::cout << "Mismatch riscontrato alla coordinata! CPU: " << val_cpu << " | GPU: " << gpu_aggregated_map[coord] << std::endl;
        }
    }

    if (pass && gpu_aggregated_map.size() == cpu_map.size()) {
        std::cout << "[SUCCESS] L'intera pipeline C++/CUDA e il costruttore automatico funzionano al 100% senza bug o crash!" << std::endl;
    } else {
        std::cout << "[FAIL] Errore di validazione numerica." << std::endl;
    }

    // Pulizia finale delle risorse hardware VRAM
    cudaFree(csf.d_vals); cudaFree(csf.d_coordinate_matrix);
    cudaFree(d_dense_in); cudaFree(d_out_indices); cudaFree(d_out_vals); cudaFree(d_global_counter);
    return 0;
}