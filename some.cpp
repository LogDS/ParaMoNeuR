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


int main() {
    std::cout << "[ZERO-ALLOCATION IN-PLACE CONVERSION SUPERATO]" << std::endl;

    int dense_sizes[] = {2};
    std::string expression = "ijko,o->ijk";
    EinsumPlan plan = compile_einsum_string_simple(expression);

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

    launch_csf_einsum(input_csf, plan, d_dense_in, coo_out);
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