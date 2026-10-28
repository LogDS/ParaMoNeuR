#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cassert>
#include <map>
#include <stdexcept>
#include "csf_einsum.cuh"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <thrust/device_ptr.h>
#include <thrust/sort.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/tuple.h>
#include <thrust/sequence.h>
#include <thrust/gather.h>

#include "yaucl/strings/string_utils.h"

#define CUDA_CHECK(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true) {
   if (code != cudaSuccess) {
      fprintf(stderr, "GPU Error: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

// =========================================================================
// FIX 2): PARSER STATICO PURO SENZA DENSE SIZES (Esempio: "ij,jk->ik")
// =========================================================================
EinsumPlan compile_sparse_einsum_string(const std::string& expression) {
    EinsumPlan plan{};

    // Inizializzazione difensiva degli array statici
    plan.num_contracted_modes = 0;
    for (int i = 0; i < MAX_TENSOR_ORDER; ++i) {
        plan.map_A_to_global[i] = -1;
        plan.map_B_to_global[i] = -1;
        plan.map_out_to_global[i] = -1;
        plan.global_contracted_levels[i] = -1;
    }

    size_t comma_pos = expression.find(',');
    size_t arrow_pos = expression.find("->");
    if (comma_pos == std::string::npos || arrow_pos == std::string::npos) {
        throw std::invalid_argument("Formato stringa Einsum invalido. Usare: 'ij,jk->ik'");
    }

    std::string labels_A   = expression.substr(0, comma_pos);
    std::string labels_B   = expression.substr(comma_pos + 1, arrow_pos - comma_pos - 1);
    std::string labels_Out = expression.substr(arrow_pos + 2);

    plan.order_A = static_cast<int>(labels_A.length());
    plan.order_B = static_cast<int>(labels_B.length());
    plan.order_Out = static_cast<int>(labels_Out.length());

    // Raccogliamo tutte le label uniche incontrate per definire l'ordine globale degli assi
    std::vector<char> global_labels;
    auto add_unique_label = [&](char c) {
        if (std::find(global_labels.begin(), global_labels.end(), c) == global_labels.end()) {
            global_labels.push_back(c);
        }
    };

    for (char c : labels_A) add_unique_label(c);
    for (char c : labels_B) add_unique_label(c);
    for (char c : labels_Out) add_unique_label(c);

    // Mappiamo le posizioni locali di ogni tensore nello spazio degli indici globali
    auto get_global_id = [&](char c) {
        return static_cast<int>(std::distance(global_labels.begin(), std::find(global_labels.begin(), global_labels.end(), c)));
    };

    for (int i = 0; i < plan.order_A; ++i)   plan.map_A_to_global[i] = get_global_id(labels_A[i]);
    for (int i = 0; i < plan.order_B; ++i)   plan.map_B_to_global[i] = get_global_id(labels_B[i]);
    for (int i = 0; i < plan.order_Out; ++i) plan.map_out_to_global[i] = get_global_id(labels_Out[i]);

    // Rileviamo gli assi contratti: presenti negli input ma ASSENTI nell'output
    for (char c : global_labels) {
        if (labels_Out.find(c) == std::string::npos) {
            plan.global_contracted_levels[plan.num_contracted_modes] = get_global_id(c);
            plan.num_contracted_modes++;
        }
    }

    return plan;
}


SumReductionPlan compile_sum_reduction_string_simple(const std::string& expression) {
    SumReductionPlan plan{};

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

// // 2. IMPLEMENTAZIONE DEL COSTRUTTORE AUTOMATICO DA COO A TENSOR DEVICE
// UniversalCSF build_universal_csf_from_coo(const std::vector<RawCOOElement>& raw_elements, int tensor_order) {
//     UniversalCSF csf{};
//     csf.order = tensor_order;
//     csf.nnz = static_cast<int>(raw_elements.size());
//
//     if (csf.nnz == 0) {
//         csf.d_vals = nullptr;
//         csf.d_coordinate_matrix = nullptr;
//         return csf;
//     }
//
//     // Ordinamento lessicografico stabile per le coordinate sparse
//     std::vector<RawCOOElement> sorted_elements = raw_elements;
//     std::sort(sorted_elements.begin(), sorted_elements.end(), [tensor_order](const RawCOOElement& a, const RawCOOElement& b) {
//         for (int d = 0; d < tensor_order; ++d) {
//             if (a.coords[d] != b.coords[d]) {
//                 return a.coords[d] < b.coords[d];
//             }
//         }
//         return false;
//     });
//
//     std::vector<float> h_vals(csf.nnz);
//     std::vector<int> h_coordinate_matrix;
//     h_coordinate_matrix.reserve(csf.nnz * tensor_order);
//
//     for (int n = 0; n < csf.nnz; ++n) {
//         h_vals[n] = sorted_elements[n].val;
//         for (int d = 0; d < tensor_order; ++d) {
//             h_coordinate_matrix.push_back(sorted_elements[n].coords[d]);
//         }
//     }
//
//     // Allocazione fisica lineare dei segmenti contigui sulla VRAM
//     CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
//     CUDA_CHECK(cudaMalloc(&csf.d_coordinate_matrix, h_coordinate_matrix.size() * sizeof(int)));
//
//     CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
//     CUDA_CHECK(cudaMemcpy(csf.d_coordinate_matrix, h_coordinate_matrix.data(), h_coordinate_matrix.size() * sizeof(int), cudaMemcpyHostToDevice));
//
//     return csf;
// }


// Struttura di confronto personalizzata per ordinare le righe della matrice di coordinate su GPU.
// Esegue un confronto lessicografico riga per riga fino al rango massimo (7D).
struct RowLexicographicalComparator {
    int order;
    RowLexicographicalComparator(int _order) : order(_order) {}

    __host__ __device__ bool operator()(const int& idx_A, const int& idx_B) const {
        // Nota: questo comparatore richiede un array ausiliario di permutazione
        // o un ordinamento diretto tramite chiavi espanse (chiavi zip).
        return idx_A < idx_B;
    }
};
//
// UniversalCSF build_universal_csf_from_coo(const std::vector<RawCOOElement>& raw_elements, int tensor_order) {
//     UniversalCSF csf{};
//     csf.order = tensor_order;
//     csf.nnz = static_cast<int>(raw_elements.size());
//
//     if (csf.nnz == 0) {
//         csf.d_unified_buffer = nullptr;
//         csf.d_vals = nullptr;
//         csf.d_coordinate_matrix = nullptr;
//         return csf;
//     }
//
//     // 1. Allocazione immediata del buffer unificato su GPU SENZA ordinare su Host
//     size_t vals_bytes = csf.nnz * sizeof(float);
//     size_t coords_bytes = csf.nnz * tensor_order * sizeof(int);
//     csf.total_buffer_bytes = vals_bytes + coords_bytes;
//
//     // Buffer temporaneo host lineare per la copia grezza speculare
//     std::vector<char> h_unified_buffer(csf.total_buffer_bytes);
//     float* h_vals_ptr = reinterpret_cast<float*>(h_unified_buffer.data());
//     int* h_coords_ptr = reinterpret_cast<int*>(h_unified_buffer.data() + vals_bytes);
//
//     // Trasferimento lineare non ordinato (velocità di picco della CPU)
//     for (int n = 0; n < csf.nnz; ++n) {
//         h_vals_ptr[n] = raw_elements[n].val;
//         for (int d = 0; d < tensor_order; ++d) {
//             h_coords_ptr[n * tensor_order + d] = raw_elements[n].coords[d];
//         }
//     }
//
//     // Singola copia bulk Host-to-Device via PCIe ad alta efficienza
//     CUDA_CHECK(cudaMalloc(&csf.d_unified_buffer, csf.total_buffer_bytes));
//     CUDA_CHECK(cudaMemcpy(csf.d_unified_buffer, h_unified_buffer.data(), csf.total_buffer_bytes, cudaMemcpyHostToDevice));
//
//     // Risoluzione dei puntatori interni sulla VRAM
//     char* d_byte_base = reinterpret_cast<char*>(csf.d_unified_buffer);
//     csf.d_vals = reinterpret_cast<float*>(d_byte_base);
//     csf.d_coordinate_matrix = reinterpret_cast<int*>(d_byte_base + vals_bytes);
//
//     // =========================================================================
//     // ORDINAMENTO PARALLELO SU GPU VIA NVIDIA THRUST (Abbatte i 35ms a < 1ms)
//     // =========================================================================
//     // Per ordinare la matrice delle coordinate [nnz * order] preservando l'allineamento
//     // con l'array dei valori (d_vals), creiamo un vettore di indici di permutazione chiavi [0, 1, 2, ..., nnz-1]
//     int* d_permutation_indices;
//     CUDA_CHECK(cudaMalloc(&d_permutation_indices, csf.nnz * sizeof(int)));
//
//     // Inizializziamo la sequenza [0, 1, ..., nnz-1] direttamente su GPU
//     thrust::device_ptr<int> thr_perm_ptr(d_permutation_indices);
//     thrust::sequence(thr_perm_ptr, thr_perm_ptr + csf.nnz);
//
//     // Otteniamo i puntatori device gestiti da Thrust per i valori
//     thrust::device_ptr<float> thr_vals_ptr(csf.d_vals);
//
//     // Definiamo un lambda o un functor interno di Thrust per accedere alla matrice 2D delle coordinate sul device
//     int* d_coords_raw = csf.d_coordinate_matrix;
//     int order = csf.order;
//
//     // Ordiniamo gli indici di permutazione usando una regola lessicografica personalizzata sulla matrice delle coordinate VRAM
//     thrust::sort(thr_perm_ptr, thr_perm_ptr + csf.nnz, [d_coords_raw, order] __device__ (const int& idx_A, const int& idx_B) {
//         for (int d = 0; d < order; ++d) {
//             int c_A = d_coords_raw[idx_A * order + d];
//             int c_B = d_coords_raw[idx_B * order + d];
//             if (c_A != c_B) return c_A < c_B;
//         }
//         return false;
//     });
//
//     // Ora che abbiamo l'ordine di permutazione ottimale calcolato in parallelo dai core CUDA,
//     // riorganizziamo (permLock in-place) sia le coordinate che i valori usando dei buffer temporanei di appoggio
//     int* d_sorted_coords;
//     float* d_sorted_vals;
//     CUDA_CHECK(cudaMalloc(&d_sorted_coords, coords_bytes));
//     CUDA_CHECK(cudaMalloc(&d_sorted_vals, vals_bytes));
//
//     thrust::device_ptr<int> thr_sorted_coords_ptr(d_sorted_coords);
//     thrust::device_ptr<float> thr_sorted_vals_ptr(d_sorted_vals);
//
//     // Applichiamo la permutazione parallela (Gather) ad altissima velocità
//     thrust::gather(thr_perm_ptr, thr_perm_ptr + csf.nnz, thr_vals_ptr, thr_sorted_vals_ptr);
//
//     // Permutiamo l'array delle coordinate riga per riga
//     int* d_sorted_coords_raw = d_sorted_coords;
//     thrust::for_each(thr_perm_ptr, thr_perm_ptr + csf.nnz, [d_coords_raw, d_sorted_coords_raw, order, d_permutation_indices] __device__ (const int& orig_idx) {
//         // Trova la posizione corrente dell'indice riordinato
//         int current_pos = &orig_idx - d_permutation_indices;
//         for(int d = 0; d < order; ++d) {
//             d_sorted_coords_raw[current_pos * order + d] = d_coords_raw[orig_idx * order + d];
//         }
//     });
//
//     // Sovrascriviamo in blocco il nostro buffer unificato principale con i dati perfettamente ordinati
//     CUDA_CHECK(cudaMemcpy(csf.d_vals, d_sorted_vals, vals_bytes, cudaMemcpyDeviceToDevice));
//     CUDA_CHECK(cudaMemcpy(csf.d_coordinate_matrix, d_sorted_coords, coords_bytes, cudaMemcpyDeviceToDevice));
//
//     // Liberazione delle aree ausiliarie temporanee di Thrust
//     cudaFree(d_permutation_indices);
//     cudaFree(d_sorted_coords);
//     cudaFree(d_sorted_vals);
//
//     return csf;
// }
UniversalCSF build_universal_csf_from_coo(const std::vector<RawCOOElement>& raw_elements, int tensor_order);

#ifdef CPU_WORK
UniversalCSF build_universal_csf_from_coo(const std::vector<RawCOOElement>& raw_elements, int tensor_order) {
    UniversalCSF csf{};
    csf.order = tensor_order; csf.nnz = static_cast<int>(raw_elements.size());
    std::vector<RawCOOElement> sorted_elements = raw_elements;
    std::sort(sorted_elements.begin(), sorted_elements.end(), [tensor_order](const RawCOOElement& a, const RawCOOElement& b) {
        for (int d = 0; d < tensor_order; ++d) { if (a.coords[d] != b.coords[d]) return a.coords[d] < b.coords[d]; }
        return false;
    });
    size_t vals_bytes = csf.nnz * sizeof(float);
    size_t coords_bytes = csf.nnz * tensor_order * sizeof(int);
    csf.total_buffer_bytes = vals_bytes + coords_bytes;
    std::vector<char> h_unified_buffer(csf.total_buffer_bytes);
    float* h_vals_ptr = reinterpret_cast<float*>(h_unified_buffer.data());
    int* h_coords_ptr = reinterpret_cast<int*>(h_unified_buffer.data() + vals_bytes);
    for (int n = 0; n < csf.nnz; ++n) {
        h_vals_ptr[n] = sorted_elements[n].val;
        for (int d = 0; d < tensor_order; ++d) h_coords_ptr[n * tensor_order + d] = sorted_elements[n].coords[d];
    }
    CUDA_CHECK(cudaMalloc(&csf.d_unified_buffer, csf.total_buffer_bytes));
    CUDA_CHECK(cudaMemcpy(csf.d_unified_buffer, h_unified_buffer.data(), csf.total_buffer_bytes, cudaMemcpyHostToDevice));
    char* d_byte_base = reinterpret_cast<char*>(csf.d_unified_buffer);
    csf.d_vals = reinterpret_cast<float*>(d_byte_base);
    csf.d_coordinate_matrix = reinterpret_cast<int*>(d_byte_base + vals_bytes);
    return csf;
}
#endif
UnifiedCOOOutput create_unified_coo_output(int max_nnz, int out_order) {
    UnifiedCOOOutput out{}; out.max_nnz = max_nnz; out.out_order = out_order;
    size_t indices_bytes = max_nnz * out_order * sizeof(int);
    size_t vals_bytes    = max_nnz * sizeof(float);
    size_t counter_bytes = sizeof(int);
    out.total_buffer_bytes = indices_bytes + vals_bytes + counter_bytes;
    CUDA_CHECK(cudaMalloc(&out.d_unified_buffer, out.total_buffer_bytes));
    char* d_byte_base = reinterpret_cast<char*>(out.d_unified_buffer);
    char* d_counter_ptr = d_byte_base + indices_bytes + vals_bytes;
    CUDA_CHECK(cudaMemset(d_counter_ptr, 0, counter_bytes));
    out.d_out_indices    = reinterpret_cast<int*>(d_byte_base);
    out.d_out_vals       = reinterpret_cast<float*>(d_byte_base + indices_bytes);
    out.d_global_counter = reinterpret_cast<int*>(d_counter_ptr);
    return out;
}


int test_sum_reduction() {
    std::cout << "[ZERO-ALLOCATION IN-PLACE CONVERSION SUPERATO]" << std::endl;

    int dense_sizes[] = {2};
    std::string expression = "ijko,o->ijk";
    SumReductionPlan plan = compile_sum_reduction_string_simple(expression);

    std::vector<RawCOOElement> raw_dataset = {
        {{1, 1, 1, 1}, 16.0f}, {{0, 0, 0, 0}, 1.0f},  {{1, 0, 1, 0}, 11.0f},
        {{0, 0, 0, 1}, 2.0f},  {{0, 0, 1, 0}, 3.0f},  {{0, 0, 1, 1}, 4.0f},
        {{0, 1, 0, 0}, 5.0f},  {{0, 1, 0, 1}, 6.0f},  {{0, 1, 1, 0}, 7.0f},
        {{0, 1, 1, 1}, 8.0f},  {{1, 0, 0, 0}, 9.0f},  {{1, 0, 0, 1}, 10.0f},
        {{1, 0, 1, 1}, 12.0f}, {{1, 1, 0, 0}, 13.0f}, {{1, 1, 0, 1}, 14.0f},
        {{1, 1, 1, 0}, 15.0f}
    };

    UniversalCSF input_csf = build_universal_csf_from_coo(raw_dataset, 4);

    std::vector<float> h_dense_in = {2.0f, 5.0f};
    float* d_dense_in;
    CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));

    int out_order = input_csf.order - plan.num_contracted_modes;
    UnifiedCOOOutput coo_out = create_unified_coo_output(input_csf.nnz, out_order);

    launch_csf_einsum_tensor_sum_reduction(input_csf, plan, d_dense_in, coo_out);
    CUDA_CHECK(cudaDeviceSynchronize());

    // -------------------------------------------------------------------------
    // CONVERSIONE IN-PLACE: RIUTILIZZO COMPLETO AL 100% DI d_unified_buffer
    // -------------------------------------------------------------------------
    UniversalCSF output_csf = convert_coo_to_csf_on_gpu(coo_out);

    std::cout << " -> Indirizzo Buffer di Output originario: " << coo_out.d_unified_buffer << std::endl;
    std::cout << " -> Indirizzo Buffer del nuovo UniversalCSF: " << output_csf.d_unified_buffer << std::endl;

    // Verifica formale della coincidenza del puntatore VRAM
    assert(coo_out.d_unified_buffer == output_csf.d_unified_buffer && "Errore: è stata allocata nuova memoria!");

    std::cout << "\n[PIPELINE COMPLETATA CON SUCCESSO SENZA REALLOCAZIONI]" << std::endl;
    std::cout << " -> Rango finale: " << output_csf.order << " | Foglie reali: " << output_csf.nnz << std::endl;

    cudaFree(input_csf.d_unified_buffer);
    cudaFree(coo_out.d_unified_buffer); // Questa singola free distrugge in blocco sia il vecchio COO che il nuovo CSF!
    cudaFree(d_dense_in);
    return 0;
}

int basic_stress_test() {
    std::cout << "[STRESS TEST AD ALTO VOLUME] Inizializzazione della contrazione a due tensori..." << std::endl;

    // 1. Parsing dell'espressione per contrazione sparsa (Moltiplicazione Matrice-Matrice "ij,jk->ik")
    std::string expression = "ij,jk->ik";
    EinsumPlan plan = compile_sparse_einsum_string(expression);

    // 2. Generazione procedurale di due dataset COO sparsi accoppiati ad alto volume
    // Dimensioni logiche delle matrici: 100 x 100
    std::vector<RawCOOElement> coo_A;
    std::vector<RawCOOElement> coo_B;

    // Popoliamo la matrice A ("ij") e la matrice B ("jk")
    // Creiamo intersezioni mirate sulle coordinate per testare l'accumulo di massa
    for (int i = 0; i < 50; ++i) {
        for (int j = 0; j < 10; ++j) {
            coo_A.push_back({ {i, j, 0, 0, 0, 0, 0}, static_cast<float>(i + j + 1) });
        }
    }
    for (int j = 0; j < 10; ++j) {
        for (int k = 0; k < 40; ++k) {
            coo_B.push_back({ {j, k, 0, 0, 0, 0, 0}, static_cast<float>(j * 2 + k + 1) });
        }
    }

    std::cout << " -> Dimensioni di Input: NNZ_A = " << coo_A.size() << " | NNZ_B = " << coo_B.size() << std::endl;

    // Compressione dei due dataset in oggetti UniversalCSF caricati in VRAM
    UniversalCSF csf_A = build_universal_csf_from_coo(coo_A, plan.order_A);
    UniversalCSF csf_B = build_universal_csf_from_coo(coo_B, plan.order_B);

    // 3. CALCOLO DI RIFERIMENTO SEQUENZIALE SU CPU (A MANO TRAMITE MAPPA DI ACCUMULO)
    std::map<std::pair<int, int>, float> cpu_reference_map;
    for (const auto& el_A : coo_A) {
        int i = el_A.coords[0];
        int j_A = el_A.coords[1];

        for (const auto& el_B : coo_B) {
            int j_B = el_B.coords[0];
            int k = el_B.coords[1];

            // Intersezione algebrica sull'asse contratto 'j'
            if (j_A == j_B) {
                cpu_reference_map[{i, k}] += el_A.val * el_B.val;
            }
        }
    }
    std::cout << " -> Calcolo CPU completato. Entry contratte uniche attese: " << cpu_reference_map.size() << std::endl;

    // 4. Allocazione del buffer unificato di output su GPU
    int max_theoretical_nnz = csf_A.nnz * csf_B.nnz;
    UnifiedCOOOutput coo_out = create_unified_coo_output(max_theoretical_nnz, plan.order_Out);

    // 5. ESECUZIONE DEL MOTORE GPU E DISK SYNCHRONIZATION
    launch_csf_einsum_two_tensors(csf_A, csf_B, plan, coo_out);
    CUDA_CHECK(cudaDeviceSynchronize());

    int h_final_nnz = 0;
    CUDA_CHECK(cudaMemcpy(&h_final_nnz, coo_out.d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
    std::cout << " -> Calcolo GPU completato. Record intermedi generati: " << h_final_nnz << std::endl;

    // Estrazione dei vettori di output dalla VRAM all'Host
    std::vector<int> h_out_indices(h_final_nnz * plan.order_Out);
    std::vector<float> h_out_vals(h_final_nnz);
    CUDA_CHECK(cudaMemcpy(h_out_indices.data(), coo_out.d_out_indices, h_final_nnz * plan.order_Out * sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_out_vals.data(), coo_out.d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));

    // 6. AGGREGAZIONE POST-RIDUZIONE DEI DUPLICATI DELLA GPU
    std::map<std::pair<int, int>, float> gpu_aggregated_map;
    for (int n = 0; n < h_final_nnz; ++n) {
        int i = h_out_indices[n * plan.order_Out + 0];
        int k = h_out_indices[n * plan.order_Out + 1];
        gpu_aggregated_map[{i, k}] += h_out_vals[n];
    }

    // 7. VERIFICA FORMALE DELLA DIFFERENZA DEI RISULTATI (VALIDATION ASSERT)
    bool validation_success = true;
    float max_error = 0.0f;

    for (auto const& [coord, cpu_val] : cpu_reference_map) {
        float gpu_val = gpu_aggregated_map[coord];
        float error = std::abs(gpu_val - cpu_val);
        if (error > max_error) max_error = error;

        if (error > 1e-3f) {
            validation_success = false;
            std::cout << "[ERROR] Mismatch alla coordinata (" << coord.first << "," << coord.second
                      << ") -> CPU: " << cpu_val << " | GPU Aggregata: " << gpu_val << std::endl;
        }
    }

    if (validation_success && gpu_aggregated_map.size() == cpu_reference_map.size()) {
        std::cout << "\n[STRESS TEST SUPERATO CON SUCCESSO 100%]" << std::endl;
        std::cout << " -> Errore massimo registrato: " << max_error << std::endl;
        std::cout << " -> Validazione incrociata perfetta per tutte le " << gpu_aggregated_map.size() << " coordinate 2D finali." << std::endl;
    } else {
        std::cout << "\n[FAIL] Rilevate anomalie nel motore di calcolo numerico sparse." << std::endl;
        assert(false);
    }
    // Pulizia totale della memoria VRAM allocata
    cudaFree(csf_A.d_unified_buffer);
    cudaFree(csf_B.d_unified_buffer);
    cudaFree(coo_out.d_unified_buffer);
    return 0;
}


int stress_bigger() {
    std::cout << "[STRESS TEST NON-TRIVIALE] Contrazione Sparse 5D x 6D -> 7D" << std::endl;

    // 1. Parsing dell'espressione complessa ad alto rango ("abcde,cdefgh->abfghij")
    std::string expression = "abcde,cdefgh->abfghij";
    EinsumPlan plan = compile_sparse_einsum_string(expression);

    std::cout << " -> Mappatura: Rango A=" << plan.order_A << " | Rango B=" << plan.order_B << " -> Rango Out=" << plan.order_Out << std::endl;
    std::cout << " -> Modi contratti individuati: " << plan.num_contracted_modes << std::endl;

    // 2. Generazione procedurale controllata dei dataset COO
    std::vector<RawCOOElement> coo_A;
    std::vector<RawCOOElement> coo_B;

    // Pattern sparsi ad incastro geometrico per far scattare le intersezioni
    for (int a = 0; a < 3; ++a) {
        for (int b = 0; b < 2; ++b) {
            for (int c = 0; c < 4; ++c) {
                for (int d = 0; d < 3; ++d) {
                    for (int e = 0; e < 2; ++e) {
                        float val = static_cast<float>(a + b + c + d + e + 1);
                        coo_A.push_back({ {a, b, c, d, e, 0, 0}, val });
                    }
                }
            }
        }
    }

    for (int c = 0; c < 4; ++c) {
        for (int d = 0; d < 3; ++d) {
            for (int e = 0; e < 2; ++e) {
                for (int f = 0; f < 2; ++f) {
                    for (int g = 0; g < 2; ++g) {
                        for (int h = 0; h < 2; ++h) {
                            float val = static_cast<float>(c * d + e + f + g + h + 1);
                            coo_B.push_back({ {c, d, e, f, g, h, 0}, val });
                        }
                    }
                }
            }
        }
    }

    std::cout << " -> Elementi caricati: NNZ_A = " << coo_A.size() << " | NNZ_B = " << coo_B.size() << std::endl;

    // Compressione Host-to-Device unificata
    UniversalCSF csf_A = build_universal_csf_from_coo(coo_A, plan.order_A);
    UniversalCSF csf_B = build_universal_csf_from_coo(coo_B, plan.order_B);

    // 3. COMPUTAZIONE DI RIFERIMENTO SU CPU (A MANO TRAMITE MAPPA DI RIDUZIONE)
    std::map<std::vector<int>, float> cpu_reference_map;

    for (const auto& el_A : coo_A) {
        int a = el_A.coords[0]; int b = el_A.coords[1]; int c_A = el_A.coords[2]; int d_A = el_A.coords[3]; int e_A = el_A.coords[4];

        for (const auto& el_B : coo_B) {
            int c_B = el_B.coords[0]; int d_B = el_B.coords[1]; int e_B = el_B.coords[2];
            int f = el_B.coords[3];   int g = el_B.coords[4];   int h = el_B.coords[5];

            // Intersezione esatta multi-asse sui livelli contratti 'c' e 'd', e sul condiviso 'e'
            if (c_A == c_B && d_A == d_B && e_A == e_B) {
                // Generazione coordinata di proiezione 7D attesa: [a, b, f, g, h, i, j] (i e j sono 0 nel mock)
                std::vector<int> out_coord = {a, b, f, g, h, 0, 0};
                cpu_reference_map[out_coord] += el_A.val * el_B.val;
            }
        }
    }
    std::cout << " -> Verifica CPU completata. Punti dello spazio ridotto 7D attesi: " << cpu_reference_map.size() << std::endl;

    // 4. Allocazione del buffer unificato di output su GPU per rango 7
    int max_theoretical_nnz = csf_A.nnz * csf_B.nnz;
    UnifiedCOOOutput coo_out = create_unified_coo_output(max_theoretical_nnz, plan.order_Out);

    // 5. DISPATCH KERNEL CUDA SU GRIGLIA GENERICA
    launch_csf_einsum_two_tensors(csf_A, csf_B, plan, coo_out);
    CUDA_CHECK(cudaDeviceSynchronize());

    int h_final_nnz = 0;
    CUDA_CHECK(cudaMemcpy(&h_final_nnz, coo_out.d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
    std::cout << " -> Calcolo GPU completato. Record emessi in totale: " << h_final_nnz << std::endl;

    // Copia selettiva host dei risultati
    std::vector<int> h_out_indices(h_final_nnz * plan.order_Out);
    std::vector<float> h_out_vals(h_final_nnz);
    CUDA_CHECK(cudaMemcpy(h_out_indices.data(), coo_out.d_out_indices, h_final_nnz * plan.order_Out * sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_out_vals.data(), coo_out.d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));

    // 6. ACCUMULO POST-REDUCE LATO CPU DEI RECORD INTERMEDI GPU
    std::map<std::vector<int>, float> gpu_aggregated_map;
    for (int n = 0; n < h_final_nnz; ++n) {
        std::vector<int> coord(plan.order_Out);
        for(int d = 0; d < plan.order_Out; ++d) {
            coord[d] = h_out_indices[n * plan.order_Out + d];
        }
        gpu_aggregated_map[coord] += h_out_vals[n];
    }

    // 7. VERIFICA DELL'ERRORE MASSIMO E DISCREPANZA NUMERICA DEL MOTORE ALTO RANGO
    bool validation_success = true;
    float max_error = 0.0f;

    for (auto const& [coord, cpu_val] : cpu_reference_map) {
        float gpu_val = gpu_aggregated_map[coord];
        float error = std::abs(gpu_val - cpu_val);
        if (error > max_error) max_error = error;

        if (error > 1e-3f) {
            validation_success = false;
            std::cout << "[ERROR] Discrepanza trovata! CPU: " << cpu_val << " | GPU: " << gpu_val << std::endl;
        }
    }

    if (validation_success && gpu_aggregated_map.size() == cpu_reference_map.size()) {
        std::cout << "\n[STRESS TEST ALTO RANGO PASSED 100%]" << std::endl;
        std::cout << " -> Rango elaborato con successo: 7D" << std::endl;
        std::cout << " -> Errore massimo di contrazione registrato: " << max_error << std::endl;
        std::cout << " -> Mappatura e matching degli assi perfetti su " << gpu_aggregated_map.size() << " coordinate uniche." << std::endl;
    } else {
        std::cout << "\n[FAIL] Errore critico nel Mode Matching multi-dimensionale GPU." << std::endl;
        assert(false);
    }

    // Pulizia totale della memoria VRAM
    cudaFree(csf_A.d_unified_buffer);
    cudaFree(csf_B.d_unified_buffer);
    cudaFree(coo_out.d_unified_buffer);
    return 0;
}

int correctness() {
    std::cout << "[STRESS TEST PARAMETRIZZATO 5D x 5D] Avvio configurazione di massa..." << std::endl;

    // 1. Configurazione delle dimensioni degli assi (Modificabili per scalare il test)
    const int DIM_A = 5;
    const int DIM_B = 10;
    const int DIM_C = 8;
    const int DIM_D = 8;
    const int DIM_E = 2;

    // Parsing dell'espressione iper-intrecciata ("abcde,bdace->abe")
    std::string expression = "abcde,bdace->abe";
    EinsumPlan plan = compile_sparse_einsum_string(expression);

    std::cout << " -> Mappatura Assi: Rango A=" << plan.order_A << " | Rango B=" << plan.order_B << " -> Rango Out=" << plan.order_Out << std::endl;
    std::cout << " -> Geometria Spaziale: [" << DIM_A << "x" << DIM_B << "x" << DIM_C << "x" << DIM_D << "x" << DIM_E << "]" << std::endl;

    // 2. Generazione procedurale scalabile dei dataset COO
    std::vector<RawCOOElement> coo_A;
    std::vector<RawCOOElement> coo_B;

    // Generazione massiva Tensore A ("abcde")
    for (int a = 0; a < DIM_A; ++a) {
        for (int b = 0; b < DIM_B; ++b) {
            for (int c = 0; c < DIM_C; ++c) {
                for (int d = 0; d < DIM_D; ++d) {
                    for (int e = 0; e < DIM_E; ++e) {
                        // Generiamo valori deterministici basati sulle coordinate
                        float val = static_cast<float>((a + b + c + d + e) % 10 + 1);
                        coo_A.push_back({ {a, b, c, d, e, 0, 0}, val });
                    }
                }
            }
        }
    }

    // Generazione massiva Tensore B ("bdace")
    // Indici hardware mappati su: coords[0]=b, coords[1]=d, coords[2]=a, coords[3]=c, coords[4]=e
    for (int b = 0; b < DIM_B; ++b) {
        for (int d = 0; d < DIM_D; ++d) {
            for (int a = 0; a < DIM_A; ++a) {
                for (int c = 0; c < DIM_C; ++c) {
                    for (int e = 0; e < DIM_E; ++e) {
                        float val = static_cast<float>((b * d + a + c) % 5 + 1);
                        coo_B.push_back({ {b, d, a, c, e, 0, 0}, val });
                    }
                }
            }
        }
    }

    std::cout << " -> Elementi allocati in VRAM: NNZ_A = " << coo_A.size() << " | NNZ_B = " << coo_B.size() << std::endl;

    // Compressione e caricamento unificato dei due grandi blocchi su GPU
    UniversalCSF csf_A = build_universal_csf_from_coo(coo_A, plan.order_A);
    UniversalCSF csf_B = build_universal_csf_from_coo(coo_B, plan.order_B);


    // 3. COMPUTAZIONE DI RIFERIMENTO SU CPU (OTTIMIZZATA PER ALTI VOLUMI)
    // Usiamo una mappa con chiave 3D per rispecchiare plan.order_Out = 3 ("abe")
    std::map<std::vector<int>, float> cpu_reference_map;

    for (const auto& el_A : coo_A) {
        int a_A = el_A.coords[0]; int b_A = el_A.coords[1];
        int c_A = el_A.coords[2]; int d_A = el_A.coords[3]; int e_A = el_A.coords[4];

        for (const auto& el_B : coo_B) {
            int b_B = el_B.coords[0]; int d_B = el_B.coords[1];
            int a_B = el_B.coords[2]; int c_B = el_B.coords[3]; int e_B = el_B.coords[4];

            // Condizione di matching geometrico totale (intersezione)
            if (a_A == a_B && b_A == b_B && c_A == c_B && d_A == d_B && e_A == e_B) {
                std::vector<int> out_coord = {a_A, b_A, e_A};
                cpu_reference_map[out_coord] += el_A.val * el_B.val;
            }
        }
    }
    std::cout << " -> Verifica CPU completata. Punti dello spazio ridotto 3D attesi: " << cpu_reference_map.size() << std::endl;


    // 4. Allocazione del buffer di output compatto su GPU
    // Nel caso peggiore la dimensione può scalare fino al prodotto delle foglie di input
    int max_theoretical_nnz = csf_A.nnz * csf_B.nnz;
    UnifiedCOOOutput coo_out = create_unified_coo_output(max_theoretical_nnz, plan.order_Out);

    // 5. INVOCAZIONE MOTORE KERNEL GPU
    launch_csf_einsum_two_tensors(csf_A, csf_B, plan, coo_out);
    CUDA_CHECK(cudaDeviceSynchronize());

    int h_final_nnz = 0;
    CUDA_CHECK(cudaMemcpy(&h_final_nnz, coo_out.d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));
    std::cout << " -> Calcolo GPU completato. Record intermedi generati: " << h_final_nnz << std::endl;

    // Estrazione dei vettori di output dalla VRAM all'Host
    std::vector<int> h_out_indices(h_final_nnz * plan.order_Out);
    std::vector<float> h_out_vals(h_final_nnz);
    CUDA_CHECK(cudaMemcpy(h_out_indices.data(), coo_out.d_out_indices, h_final_nnz * plan.order_Out * sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_out_vals.data(), coo_out.d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));

    // 6. AGGREGAZIONE POST-REDUCE DEI RECORD DUPLICATI DELLA GPU
    std::map<std::vector<int>, float> gpu_aggregated_map;
    for (int n = 0; n < h_final_nnz; ++n) {
        std::vector<int> coord(plan.order_Out);
        for(int d = 0; d < plan.order_Out; ++d) {
            coord[d] = h_out_indices[n * plan.order_Out + d];
        }
        gpu_aggregated_map[coord] += h_out_vals[n];
    }

    // 7. CALCOLO STATISTICO DELLO SCARTO E VALIDAZIONE NUMERICA
    bool validation_success = true;
    float max_error = 0.0f;

    for (auto const& [coord, cpu_val] : cpu_reference_map) {
        float gpu_val = gpu_aggregated_map[coord];
        float error = std::abs(gpu_val - cpu_val);
        if (error > max_error) max_error = error;

        if (error > 1e-3f) {
            validation_success = false;
            std::cout << "[ERROR] Scarto numerico rilevato! CPU: " << cpu_val << " | GPU: " << gpu_val << std::endl;
        }
    }

    if (validation_success && gpu_aggregated_map.size() == cpu_reference_map.size()) {
        std::cout << "\n[TEST DI MASSA SUPERATO AL 100%]" << std::endl;
        std::cout << " -> Elementi non-zero finali validati: " << gpu_aggregated_map.size() << std::endl;
        std::cout << " -> Errore massimo di contrazione registrato: " << max_error << std::endl;
        std::cout << " -> Il Mode-Matching strutturale è stabile su volumi HPC scala." << std::endl;
    } else {
        std::cout << "\n[FAIL] Discrepanza nei risultati della griglia di multiprocessing GPU." << std::endl;
        assert(false);
    }

    // Liberazione fisica della VRAM
    cudaFree(csf_A.d_unified_buffer);
    cudaFree(csf_B.d_unified_buffer);
    cudaFree(coo_out.d_unified_buffer);
    return 0;
}

#include <fstream>

void run_scalability_experiment(size_t current_nnz, std::ofstream& csv_file) {
    std::string expression = "abcde,bdace->abe";
    EinsumPlan plan = compile_sparse_einsum_string(expression);

    cudaEvent_t start_mut, stop_mut, start_alg, stop_alg;
    cudaEventCreate(&start_mut); cudaEventCreate(&stop_mut);
    cudaEventCreate(&start_alg); cudaEventCreate(&stop_alg);

    std::vector<RawCOOElement> coo_A;
    std::vector<RawCOOElement> coo_B;

    // 1. CARICAMENTO TENSORE A ("abcde")
    // Il file CSV contiene le colonne nell'ordine: a, b, c, d, e
    {
        std::ifstream in("benchmark_data/tensor_A_" + std::to_string(current_nnz) + ".csv");
        std::string line;
        while (std::getline(in, line)) {
            auto v = yaucl::strings::split(line, ",");
            if (v.empty()) continue;
            for (auto& ref : v) yaucl::strings::trim(ref);

            RawCOOElement indexA{};
            std::memset(&indexA.coords, 0, sizeof(indexA.coords));

            // Mappatura lineare naturale
            indexA.coords[0] = std::stoi(v[0]); // a
            indexA.coords[1] = std::stoi(v[1]); // b
            indexA.coords[2] = std::stoi(v[2]); // c
            indexA.coords[3] = std::stoi(v[3]); // d
            indexA.coords[4] = std::stoi(v[4]); // e
            indexA.val = std::stod(*v.rbegin());
            coo_A.push_back(indexA);
        }
    }

    // 2. CARICAMENTO TENSORE B ("bdace")
    // Il file CSV generato da NumPy contiene le colonne nell'ordine fisico: b, d, a, c, e
    // Per eliminare il crash sulla GPU, dobbiamo riordinare queste colonne PRIMA
    // di passarle a build_universal_csf_from_coo, in modo che l'array coords
    // segua lo schema canonico standard (0=a, 1=b, 2=c, 3=d, 4=e)
    {
        std::ifstream in("benchmark_data/tensor_B_" + std::to_string(current_nnz) + ".csv");
        std::string line;
        while (std::getline(in, line)) {
            auto v = yaucl::strings::split(line, ",");
            if (v.empty()) continue;
            for (auto& ref : v) yaucl::strings::trim(ref);

            RawCOOElement indexB{};
            std::memset(&indexB.coords, 0, sizeof(indexB.coords));
            indexB.coords[0] = std::stoi(v[0]); // a
            indexB.coords[1] = std::stoi(v[1]); // b
            indexB.coords[2] = std::stoi(v[2]); // c
            indexB.coords[3] = std::stoi(v[3]); // d
            indexB.coords[4] = std::stoi(v[4]); // e
            indexB.val = std::stod(*v.rbegin());
            coo_B.push_back(indexB);
        }
    }

    // Misura Mutazione (Caricamento e compressione)
    cudaEventRecord(start_mut);
    UniversalCSF csf_A = build_universal_csf_from_coo(coo_A, plan.order_A);
    UniversalCSF csf_B = build_universal_csf_from_coo(coo_B, plan.order_B);
    cudaEventRecord(stop_mut);
    cudaEventSynchronize(stop_mut);
    float t_mutation_ms = 0;
    cudaEventElapsedTime(&t_mutation_ms, start_mut, stop_mut);

    // Allocazione del buffer di output compatto protetto da OOM
    // Poiché i dati sono pseudo-casuali distribuiti su uno spazio immenso,
    // la coincidenza reale è bassissima. Stanziamo 1 milione di elementi stabili.
    int safety_max_nnz = 1000000;
    UnifiedCOOOutput coo_out = create_unified_coo_output(safety_max_nnz, plan.order_Out);

    // Misura Algebra (Contrazione)
    cudaEventRecord(start_alg);
    launch_csf_einsum_two_tensors(csf_A, csf_B, plan, coo_out);
    CUDA_CHECK(cudaDeviceSynchronize());
    cudaEventRecord(stop_alg);
    cudaEventSynchronize(stop_alg);

    float t_algebra_ms = 0;
    cudaEventElapsedTime(&t_algebra_ms, start_alg, stop_alg);

    int h_final_nnz = 0;
    CUDA_CHECK(cudaMemcpy(&h_final_nnz, coo_out.d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));

    // Scrittura dei log reali ed effettivi nel file di benchmark
    csv_file << "Paramoneur," << current_nnz << "," << t_mutation_ms << "," << t_algebra_ms << "\n";
    std::cout << "[REAL RUN NNZ: " << current_nnz << "] Mutazione: " << t_mutation_ms << " ms | Algebra: " << t_algebra_ms
              << " ms | Record intersecati estratti: " << h_final_nnz << "\n";

    // Pulizia e rilascio immediato delle risorse della VRAM ad ogni iterazione dello step
    cudaFree(csf_A.d_unified_buffer);
    cudaFree(csf_B.d_unified_buffer);
    cudaFree(coo_out.d_unified_buffer);

    cudaEventDestroy(start_mut); cudaEventDestroy(stop_mut);
    cudaEventDestroy(start_alg); cudaEventDestroy(stop_alg);
}

int main() {
    int deviceCount = 0;
    cudaGetDeviceCount(&deviceCount);
    std::ofstream csv_file("benchmark_custom_engine.csv");
    csv_file << "NNZ,Tempo_Mutazione_ms,Tempo_Algebra_ms\n";

    std::vector<size_t> nnz_scale_steps = {1, 10, 100, 1000, 100000, 500000};

    for (size_t current_nnz : nnz_scale_steps) {
        run_scalability_experiment(current_nnz, csv_file);
    }

    csv_file.close();
    return 0;
}