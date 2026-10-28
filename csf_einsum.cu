#include "csf_einsum.cuh"

/**
 * Kernel CUDA Universale. Riceve le due strutture dati unificate.
 * Legge dall'input unificato e scrive nell'output unificato senza overhead.
 */
__global__ void einsum_universal_leaf_kernel(
    const float* __restrict__ csf_vals,
    const float* __restrict__ dense_input,
    const int* __restrict__ coordinate_matrix,
    EinsumPlan plan,
    UniversalCSF csf,
    int* __restrict__ coo_out_indices,
    float* __restrict__ coo_out_vals,
    int* __restrict__ global_output_counter,
    int out_order)
{
    int leaf_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (leaf_idx >= csf.nnz) return;

    int current_indices[MAX_TENSOR_ORDER];

    for (int d = 0; d < csf.order; ++d) {
        current_indices[d] = coordinate_matrix[leaf_idx * csf.order + d];
    }

    int current_dense_idx = 0;
    for (int c = 0; c < plan.num_contracted_modes; ++c) {
        int target_lvl = plan.contracted_levels[c];
        current_dense_idx += current_indices[target_lvl] * plan.dense_dim_strides[c];
    }

    float calcolo = csf_vals[leaf_idx] * dense_input[current_dense_idx];

    if (calcolo != 0.0f) {
        // Scrittura atomica usando il puntatore unificato integrato
        int out_slot = atomicAdd(global_output_counter, 1);
        coo_out_vals[out_slot] = calcolo;

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
    UnifiedCOOOutput& out)
{
    int threads = 128;
    int blocks = (csf.nnz + threads - 1) / threads;

    einsum_universal_leaf_kernel<<<blocks, threads>>>(
        csf.d_vals,
        d_dense_input,
        csf.d_coordinate_matrix,
        plan,
        csf,
        out.d_out_indices,
        out.d_out_vals,
        out.d_global_counter,
        out.out_order
    );
}
