#include "csf_einsum.cuh"

/**
 * Kernel CUDA Universale e Dinamico guidato al 100% dal Piano Einsum.
 * Parallelizzazione atomica sulle foglie (NNZ) per eliminare la Warp Divergence.
 */
__global__ void einsum_universal_leaf_kernel(
    const float* __restrict__ csf_vals,
    const float* __restrict__ dense_input,
    const int* __restrict__ coordinate_matrix,
    EinsumPlan plan,
    UniversalCSF csf,
    int* __restrict__ coo_out_indices,
    float* __restrict__ coo_out_vals,
    int* __restrict__ global_output_counter)
{
    // Ogni thread elabora linearmente un elemento non-zero (foglia) del tensore
    int leaf_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (leaf_idx >= csf.nnz) return;

    int current_indices[MAX_TENSOR_ORDER];

    // 1. Estrazione coalesciuta e sicura di tutte le coordinate dell'elemento
    for (int d = 0; d < csf.order; ++d) {
        current_indices[d] = coordinate_matrix[leaf_idx * csf.order + d];
    }

    // 2. Calcolo dinamico dell'indirizzo sull'operando denso basato sui metadati del piano
    int current_dense_idx = 0;
    for (int c = 0; c < plan.num_contracted_modes; ++c) {
        int target_lvl = plan.contracted_levels[c];
        current_dense_idx += current_indices[target_lvl] * plan.dense_dim_strides[c];
    }

    // 3. Contrazione algebrica
    float calcolo = csf_vals[leaf_idx] * dense_input[current_dense_idx];

    if (calcolo != 0.0f) {
        // Scrittura atomica thread-safe nel buffer intermedio COO
        int out_slot = atomicAdd(global_output_counter, 1);
        coo_out_vals[out_slot] = calcolo;

        // Scrittura selettiva delle sole coordinate superstiti (esterne / non contratte)
        int out_order = csf.order - plan.num_contracted_modes;
        int out_d = 0;
        for (int d = 0; d < csf.order; ++d) {
            bool is_contracted = false;
            for (int c = 0; c < plan.num_contracted_modes; ++c) {
                if (plan.contracted_levels[c] == d) {
                    is_contracted = true;
                    break;
                }
            }
            if (!is_contracted) {
                coo_out_indices[out_slot * out_order + out_d] = current_indices[d];
                out_d++;
            }
        }
    }
}

void launch_csf_einsum(
    const UniversalCSF& csf,
    const EinsumPlan& plan,
    const float* d_dense_input,
    int* d_out_indices,
    float* d_out_vals,
    int* d_global_counter)
{
    // Configurazione ottimale dei blocchi in base al numero reale di elementi non-zero
    int threads = 128;
    int blocks = (csf.nnz + threads - 1) / threads;

    einsum_universal_leaf_kernel<<<blocks, threads>>>(
        csf.d_vals,
        d_dense_input,
        csf.d_coordinate_matrix,
        plan,
        csf,
        d_out_indices,
        d_out_vals,
        d_global_counter
    );
}
