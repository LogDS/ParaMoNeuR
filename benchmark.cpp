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

#define CUDA_CHECK(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true) {
   if (code != cudaSuccess) {
      fprintf(stderr, "GPU Error: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

#include <string>
#include <stdexcept>

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


int main() {
    std::cout << "[TEST] Avvio del test di verifica per Einsum CSF ad ordine arbitrario..." << std::endl;

    // ==========================================================
    // 1. COMPILAZIONE DELL'ESPRESSIONE TRAMITE PRIMITIVA
    // ==========================================================
    std::string expression = "ijko,o->ijk";
    EinsumPlan plan = compile_einsum_string_simple(expression);

    // ==========================================================
    // 2. CONFIGURAZIONE DEI DATI DI INPUT (CSF E VETTORE DENSO)
    // ==========================================================
    // UniversalCSF csf{};
    // csf.order = 4;
    // csf.nnz = 4;
    //
    // // Struttura fissa di un albero CSF linearizzato 4D per il test
    // // Rappresenta strutturalmente 4 nodi foglia validi
    // std::vector<int> h_all_ptrs = {0, 2, 4, 0, 1, 3, 5, 8};
    // std::vector<int> h_all_indices = {0, 1, 0, 1, 2, 0, 1, 0, 1, 0, 1};
    //
    // // Assegniamo valori sparse ben definiti per il test
    // std::vector<float> h_vals = {10.0f, 20.0f, 30.0f, 40.0f};
    //
    // // Impostazione degli array statici di offset
    // for(int i = 0; i < MAX_TENSOR_ORDER; ++i) {
    //     csf.ptr_offsets[i] = 0;
    //     csf.ind_offsets[i] = 0;
    // }
    // csf.ptr_offsets[0] = 0; csf.ptr_offsets[1] = 3;
    // csf.ind_offsets[0] = 0; csf.ind_offsets[1] = 2; csf.ind_offsets[2] = 5; csf.ind_offsets[3] = 8;
    UniversalCSF csf{};
    csf.order = 4;
    csf.nnz = 2; // Testiamo con 2 elementi non zeri reali nel fogliame

    // Struttura CSF 4D valida:
    // Livello 0 (i): 2 nodi radice -> puntano alle posizioni [0,1) e [1,2) del Livello 1
    // Livello 1 (j): 2 nodi -> puntano alle posizioni [0,1) e [1,2) del Livello 2
    // Livello 2 (k): 2 nodi -> puntano alle posizioni [0,1) e [1,2) del Livello 3 (Foglie)
    // Livello 3 (o): 2 nodi finali (Valori)
    std::vector<int> h_all_ptrs = {
        0, 1, 2, // Livello 0: ptr_offset = 0 (Dimensione: root_size + 1 = 3)
        0, 1, 2, // Livello 1: ptr_offset = 3
        0, 1, 2, // Livello 2: ptr_offset = 6
        0, 1, 2  // Livello 3: ptr_offset = 9
    };

    std::vector<int> h_all_indices = {
        0, 1, // Livello 0: indici 'i' -> ind_offset = 0
        0, 1, // Livello 1: indici 'j' -> ind_offset = 2
        0, 1, // Livello 2: indici 'k' -> ind_offset = 4
        0, 1  // Livello 3: indici 'o' -> ind_offset = 6
    };

    // Valori associati alle foglie
    std::vector<float> h_vals = {10.0f, 20.0f};

    // Inizializzazione pulita a zero degli array statici degli offset
    for(int i = 0; i < MAX_TENSOR_ORDER; ++i) {
        csf.ptr_offsets[i] = 0;
        csf.ind_offsets[i] = 0;
    }

    // Mappatura degli offset statici per navigare i vettori linearizzati sopra descritti
    csf.ptr_offsets[0] = 0;
    csf.ptr_offsets[1] = 3;
    csf.ptr_offsets[2] = 6;
    csf.ptr_offsets[3] = 9;

    csf.ind_offsets[0] = 0;
    csf.ind_offsets[1] = 2;
    csf.ind_offsets[2] = 4;
    csf.ind_offsets[3] = 6;

    // Allocazione e copia dei dati del Tensore CSF sulla GPU
    CUDA_CHECK(cudaMalloc(&csf.d_all_ptrs, h_all_ptrs.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&csf.d_all_indices, h_all_indices.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(csf.d_all_ptrs, h_all_ptrs.data(), h_all_ptrs.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(csf.d_all_indices, h_all_indices.data(), h_all_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));

    // Input denso (Vettore associato alla coordinata 'o', con dimensione 2)
    // Se la coordinata foglia 'o' è 0 moltiplica per 2.0, se è 1 moltiplica per 5.0
    std::vector<float> h_dense_in = {2.0f, 5.0f};
    float* d_dense_in;
    CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));

    // ==========================================================
    // 3. ALLOCAZIONE DEI BUFFER DI OUTPUT (COO INTERMEDIO/FINALE)
    // ==========================================================
    int* d_out_indices;
    float* d_out_vals;
    int* d_global_counter;

    CUDA_CHECK(cudaMalloc(&d_out_indices, csf.nnz * csf.order * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out_vals, csf.nnz * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
    CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));

    // ==========================================================
    // 4. INVOCAZIONE E CONTROLLO DEI RISULTATI
    // ==========================================================
    int root_size = 2; // Numero di nodi radice al livello 0
    launch_csf_einsum(csf, plan, d_dense_in, root_size, d_out_indices, d_out_vals, d_global_counter);
    CUDA_CHECK(cudaDeviceSynchronize());

    // Recupero del numero totale di non-zeri (NNZ) generati dall'output COO
    int h_final_nnz = 0;
    CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));

    // Recupero dei buffer COO dalla GPU all'Host
    std::vector<int> h_out_indices(h_final_nnz * csf.order);
    std::vector<float> h_out_vals(h_final_nnz);
    CUDA_CHECK(cudaMemcpy(h_out_indices.data(), d_out_indices, h_final_nnz * csf.order * sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_out_vals.data(), d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));

    // ==========================================================
    // 5. VALIDAZIONE FORMALE AUTOMATICA (ASSERT)
    // ==========================================================
    std::cout << "\n--- Verifica Elementi Generati ---" << std::endl;
    bool test_passed = true;

    for (int n = 0; n < h_final_nnz; ++n) {
        int i = h_out_indices[n * csf.order + 0];
        int j = h_out_indices[n * csf.order + 1];
        int k = h_out_indices[n * csf.order + 2];
        int o = h_out_indices[n * csf.order + 3]; // Coordinata contratta preservata nel COO intermedio
        float val_ottenuto = h_out_vals[n];

        // Calcolo del valore atteso teorico
        // Recuperiamo il valore sparse originale basandoci sull'indice dell'elemento nel test
        float val_originale = h_vals[n];
        float moltiplicatore_denso = h_dense_in[o];
        float val_atteso = val_originale * moltiplicatore_denso;

        std::cout << "COO [" << n << "] -> Coordinata: (" << i << "," << j << "," << k << "," << o
                  << ") | Calcolato: " << val_ottenuto << " | Atteso: " << val_atteso << std::endl;

        // Controllo numerico di precisione
        if (std::abs(val_ottenuto - val_atteso) > 1e-5f) {
            test_passed = false;
        }
    }

    if (test_passed && h_final_nnz > 0) {
        std::cout << "\n[TEST SUPERATO SUCCESS] Tutti i valori intermedi COO combaciano al 100%." << std::endl;
    } else {
        std::cout << "\n[TEST FALLITO ERROR] Discrepanza rilevata nei calcoli GPU o nessun elemento generato." << std::endl;
        assert(false);
    }

    // Pulizia risorse GPU
    cudaFree(csf.d_all_ptrs); cudaFree(csf.d_all_indices); cudaFree(csf.d_vals);
    cudaFree(d_dense_in); cudaFree(d_out_indices); cudaFree(d_out_vals); cudaFree(d_global_counter);

    return 0;
}

//
// int main() {
//     // ==========================================
//     // 1. CONFIGURAZIONE STATICA DEL PIANO EINSUM
//     // ==========================================
//     // Esempio: Contrazione di un tensore 4D su asse 3 ("ijko,o->ijk")
//     // EinsumPlan plan{};
//     // plan.tensor_order = 4;
//     // plan.num_contracted_modes = 1;
//     //
//     // // Inizializzazione degli array statici dell'host
//     // for(int i = 0; i < MAX_TENSOR_ORDER; ++i) {
//     //     plan.contracted_levels[i] = 0;
//     //     plan.dense_dim_strides[i] = 0;
//     // }
//     // plan.contracted_levels[0] = 3;
//     // plan.dense_dim_strides[0] = 1;
//     // Compiliamo la stringa classica
//     std::string einsum_expr = "ijko,o->ijk";
//     EinsumPlan plan = compile_einsum_string_simple(einsum_expr);
//
//     // Stampa di controllo per verificare la compilazione statica
//     std::cout << "--- Piano Einsum Compilato ---" << std::endl;
//     std::cout << "Rango Tensore Sparse: " << plan.tensor_order << std::endl;
//     std::cout << "Assi Contratti totali: " << plan.num_contracted_modes << std::endl;
//     for(int i = 0; i < plan.num_contracted_modes; ++i) {
//         std::cout << " -> Livello CSF contratto: " << plan.contracted_levels[i]
//                   << " con Stride denso = " << plan.dense_dim_strides[i] << std::endl;
//     }
//
//     // ==========================================
//     // 2. PREPARAZIONE E MOCK DATA IN INGRESSO (CSF)
//     // ==========================================
//     UniversalCSF csf{};
//     csf.order = 4;
//     csf.nnz = 8;
//
//     // Rappresentazione lineare concatenata di un albero CSF fittizio
//     std::vector<int> h_all_ptrs = {0, 2, 4, 0, 1, 3, 5, 8};
//     std::vector<int> h_all_indices = {0, 1, 0, 1, 2, 0, 1, 0, 1, 0, 1};
//     std::vector<float> h_vals(csf.nnz, 2.5f);
//
//     // Configurazione degli array di offset statici
//     csf.ptr_offsets[0] = 0; csf.ptr_offsets[1] = 3; csf.ptr_offsets[2] = 0; csf.ptr_offsets[3] = 0;
//     csf.ind_offsets[0] = 0; csf.ind_offsets[1] = 2; csf.ind_offsets[2] = 5; csf.ind_offsets[3] = 8;
//
//     // Allocazione ed invio delle strutture Dati sul Device
//     CUDA_CHECK(cudaMalloc(&csf.d_all_ptrs, h_all_ptrs.size() * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&csf.d_all_indices, h_all_indices.size() * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
//
//     CUDA_CHECK(cudaMemcpy(csf.d_all_ptrs, h_all_ptrs.data(), h_all_ptrs.size() * sizeof(int), cudaMemcpyHostToDevice));
//     CUDA_CHECK(cudaMemcpy(csf.d_all_indices, h_all_indices.data(), h_all_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
//     CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
//
//     // Input denso (Vettore di dimensione 2)
//     std::vector<float> h_dense_in = {2.0f, 2.0f};
//     float* d_dense_in;
//     CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
//     CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));
//
//     // ==========================================
//     // 3. ALLOCAZIONE DEI BUFFER COO DI OUTPUT
//     // ==========================================
//     int* d_out_indices;
//     float* d_out_vals;
//     int* d_global_counter;
//
//     // Lo spazio allocato supporta nel caso peggiore l'intera dimensione dei nodi foglia
//     CUDA_CHECK(cudaMalloc(&d_out_indices, csf.nnz * csf.order * sizeof(int)));
//     CUDA_CHECK(cudaMalloc(&d_out_vals, csf.nnz * sizeof(float)));
//     CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
//     CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));
//
//     // ==========================================
//     // 4. ESECUZIONE
//     // ==========================================
//     int root_size = 2; // Numero di entry point al livello Radice (0)
//
//     launch_csf_einsum(csf, plan, d_dense_in, root_size, d_out_indices, d_out_vals, d_global_counter);
//     CUDA_CHECK(cudaDeviceSynchronize());
//
//     // Verifica dei risultati estraendo il contatore COO generato
//     int h_final_nnz = 0;
//     CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
//
//     std::cout << "[SUCCESS] Einsum calcolato con successo utilizzando array statici.\n";
//     std::cout << "Elementi validi scritti nel buffer COO di output: " << h_final_nnz << std::endl;
//
//     // Liberazione memoria GPU
//     cudaFree(csf.d_all_ptrs);
//     cudaFree(csf.d_all_indices);
//     cudaFree(csf.d_vals);
//     cudaFree(d_dense_in);
//     cudaFree(d_out_indices);
//     cudaFree(d_out_vals);
//     cudaFree(d_global_counter);
//
//     return 0;
// }
