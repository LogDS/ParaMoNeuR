#ifndef CSF_EINSUM_CUH
#define CSF_EINSUM_CUH

#include <cuda_runtime.h>
#include <string>

#define MAX_TENSOR_ORDER 7

// Contiene i metadati dinamici estratti dal parser host
struct EinsumPlan {
    int tensor_order;
    int num_contracted_modes;
    int contracted_levels[MAX_TENSOR_ORDER];
    int dense_dim_strides[MAX_TENSOR_ORDER];
};

// Struttura CSF Universale robusta per il multiprocessing parallelo
struct UniversalCSF {
    int order;
    int nnz;
    float* d_vals;           // Valori float delle foglie (Dimensione: nnz)

    // Matrice delle coordinate [nnz * order] allocata sul device.
    // Ogni riga contiene le coordinate esatte (i, j, k, o...) di quella foglia.
    int* d_coordinate_matrix;
};

// Primitive Host C++
EinsumPlan compile_einsum_string(const std::string& expression, const int* dense_sizes);

void launch_csf_einsum(
    const UniversalCSF& csf,
    const EinsumPlan& plan,
    const float* d_dense_input,
    int* d_out_indices,
    float* d_out_vals,
    int* d_global_counter
);

#endif // CSF_EINSUM_CUH
