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

// Compilatore Host reale: estrae strides e coordinate contratte da QUALSIASI stringa compatibile
EinsumPlan compile_einsum_string(const std::string& expression, const int* dense_sizes) {
    EinsumPlan plan{};
    plan.tensor_order = 0;
    plan.num_contracted_modes = 0;
    for (int i = 0; i < MAX_TENSOR_ORDER; ++i) {
        plan.contracted_levels[i] = -1;
        plan.dense_dim_strides[i] = 0;
    }

    size_t comma_pos = expression.find(',');
    size_t arrow_pos = expression.find("->");
    if (comma_pos == std::string::npos || arrow_pos == std::string::npos) {
        throw std::invalid_argument("Formato stringa Einsum non valido.");
    }

    std::string sparse_labels = expression.substr(0, comma_pos);
    std::string dense_labels  = expression.substr(comma_pos + 1, arrow_pos - comma_pos - 1);
    std::string output_labels = expression.substr(arrow_pos + 2);

    plan.tensor_order = static_cast<int>(sparse_labels.length());

    for (int i = 0; i < plan.tensor_order; ++i) {
        char current_label = sparse_labels[i];
        if (output_labels.find(current_label) == std::string::npos) {
            plan.contracted_levels[plan.num_contracted_modes] = i;

            int stride = 1;
            size_t dense_pos = dense_labels.find(current_label);
            if (dense_pos != std::string::npos) {
                for (size_t d = dense_labels.length() - 1; d > dense_pos; --d) {
                    stride *= dense_sizes[d];
                }
                plan.dense_dim_strides[plan.num_contracted_modes] = stride;
            }
            plan.num_contracted_modes++;
        }
    }
    return plan;
}



int main() {
    std::cout << "[STRESS TEST UNIVERSALE] Avvio del motore Einsum Foglia-Centrico..." << std::endl;

    // Definiamo le dimensioni dell'operando denso (es. asse o = 2)
    int dense_sizes[] = {2};
    std::string expression = "ijko,o->ijk";
    EinsumPlan plan = compile_einsum_string(expression, dense_sizes);

    UniversalCSF csf{};
    csf.order = 4;
    csf.nnz = 16; // Stress-test ad alto volume con 16 foglie reali distinte

    // Generazione della matrice esplicita delle coordinate sull'host [16 nodi * 4 dimensioni]
    std::vector<int> h_coordinate_matrix;
    std::vector<float> h_vals(csf.nnz);

    // Mappa CPU di riferimento strutturata per validare lo spazio contratto (i,j,k)
    std::map<std::vector<int>, float> cpu_map;
    std::vector<float> h_dense_in = {2.0f, 5.0f};

    int val_idx = 0;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                for (int o = 0; o < 2; ++o) {
                    // Costruzione esplicita della riga di coordinate
                    h_coordinate_matrix.push_back(i);
                    h_coordinate_matrix.push_back(j);
                    h_coordinate_matrix.push_back(k);
                    h_coordinate_matrix.push_back(o);

                    float v = (float)(val_idx + 1);
                    h_vals[val_idx] = v;

                    // La CPU calcola e accumula nello spazio ridotto intermediario
                    cpu_map[{i, j, k}] += v * h_dense_in[o];
                    val_idx++;
                }
            }
        }
    }

    // Allocazione e copia su GPU
    CUDA_CHECK(cudaMalloc(&csf.d_vals, csf.nnz * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&csf.d_coordinate_matrix, h_coordinate_matrix.size() * sizeof(int)));

    CUDA_CHECK(cudaMemcpy(csf.d_vals, h_vals.data(), csf.nnz * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(csf.d_coordinate_matrix, h_coordinate_matrix.data(), h_coordinate_matrix.size() * sizeof(int), cudaMemcpyHostToDevice));

    float* d_dense_in;
    CUDA_CHECK(cudaMalloc(&d_dense_in, h_dense_in.size() * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d_dense_in, h_dense_in.data(), h_dense_in.size() * sizeof(float), cudaMemcpyHostToDevice));

    // Buffer di Output COO intermedio contratto (Rango finale = 4 - 1 = 3)
    int out_order = csf.order - plan.num_contracted_modes;
    int* d_out_indices; float* d_out_vals; int* d_global_counter;

    CUDA_CHECK(cudaMalloc(&d_out_indices, csf.nnz * out_order * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out_vals, csf.nnz * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_global_counter, sizeof(int)));
    CUDA_CHECK(cudaMemset(d_global_counter, 0, sizeof(int)));

    // Esecuzione
    launch_csf_einsum(csf, plan, d_dense_in, d_out_indices, d_out_vals, d_global_counter);
    CUDA_CHECK(cudaDeviceSynchronize());

    int h_final_nnz = 0;
    CUDA_CHECK(cudaMemcpy(&h_final_nnz, d_global_counter, sizeof(int), cudaMemcpyDeviceToHost));

    std::cout << " -> Calcolo GPU completato. Record intermedi COO scritti: " << h_final_nnz << std::endl;

    // Controllo difensivo di sicurezza
    if (h_final_nnz == 0) {
        std::cout << "[FAIL] La GPU non ha emesso elementi validi." << std::endl;
        return -1;
    }

    std::vector<int> h_out_indices(h_final_nnz * out_order);
    std::vector<float> h_out_vals(h_final_nnz);

    // ORA FUNZIONERÀ AL 100%: h_final_nnz > 0 e gli array sono pre-allocati al millimetro
    CUDA_CHECK(cudaMemcpy(h_out_indices.data(), d_out_indices, h_final_nnz * out_order * sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_out_vals.data(), d_out_vals, h_final_nnz * sizeof(float), cudaMemcpyDeviceToHost));

    // Validazione dei risultati numerici accumulando i record COO intermedi aventi la stessa coordinata
    std::map<std::vector<int>, float> gpu_aggregated_map;
    for(int n = 0; n < h_final_nnz; ++n) {
        std::vector<int> coord = {h_out_indices[n * out_order], h_out_indices[n * out_order + 1], h_out_indices[n * out_order + 2]};
        gpu_aggregated_map[coord] += h_out_vals[n];
    }

    bool pass = true;
    for (auto const& [coord, val_cpu] : cpu_map) {
        if (std::abs(gpu_aggregated_map[coord] - val_cpu) > 1e-3f) {
            pass = false;
            std::cout << "Mismatch riscontrato alla coordinata! CPU: " << val_cpu << " | GPU aggregata: " << gpu_aggregated_map[coord] << std::endl;
        }
    }

    if (pass) {
        std::cout << "[SUCCESS] Pipeline blindata superata al 100%! cudaMemcpy eseguita correttamente senza alcun crash." << std::endl;
    } else {
        std::cout << "[FAIL] Errore numerico nell'accumulo." << std::endl;
    }

    cudaFree(csf.d_vals); cudaFree(csf.d_coordinate_matrix);
    cudaFree(d_dense_in); cudaFree(d_out_indices); cudaFree(d_out_vals); cudaFree(d_global_counter);
    return 0;
}
