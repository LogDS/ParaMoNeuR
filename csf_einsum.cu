// #include "csf_einsum.cuh"
//
// /**
//  * Kernel CUDA Universale. Riceve le due strutture dati unificate.
//  * Legge dall'input unificato e scrive nell'output unificato senza overhead.
//  */
// __global__ void einsum_universal_leaf_kernel(
//     const float* __restrict__ csf_vals,
//     const float* __restrict__ dense_input,
//     const int* __restrict__ coordinate_matrix,
//     EinsumPlan plan,
//     UniversalCSF csf,
//     int* __restrict__ coo_out_indices,
//     float* __restrict__ coo_out_vals,
//     int* __restrict__ global_output_counter,
//     int out_order)
// {
//     int leaf_idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (leaf_idx >= csf.nnz) return;
//
//     int current_indices[MAX_TENSOR_ORDER];
//
//     for (int d = 0; d < csf.order; ++d) {
//         current_indices[d] = coordinate_matrix[leaf_idx * csf.order + d];
//     }
//
//     int current_dense_idx = 0;
//     for (int c = 0; c < plan.num_contracted_modes; ++c) {
//         int target_lvl = plan.contracted_levels[c];
//         current_dense_idx += current_indices[target_lvl] * plan.dense_dim_strides[c];
//     }
//
//     float calcolo = csf_vals[leaf_idx] * dense_input[current_dense_idx];
//
//     if (calcolo != 0.0f) {
//         // Scrittura atomica usando il puntatore unificato integrato
//         int out_slot = atomicAdd(global_output_counter, 1);
//         coo_out_vals[out_slot] = calcolo;
//
//         int out_d = 0;
//         for (int d = 0; d < csf.order; ++d) {
//             bool is_contracted = false;
//             for (int c = 0; c < plan.num_contracted_modes; ++c) {
//                 if (plan.contracted_levels[c] == d) {
//                     is_contracted = true;
//                     break;
//                 }
//             }
//             if (!is_contracted) {
//                 coo_out_indices[out_slot * out_order + out_d] = current_indices[d];
//                 out_d++;
//             }
//         }
//     }
// }
//
// void launch_csf_einsum(
//     const UniversalCSF& csf,
//     const EinsumPlan& plan,
//     const float* d_dense_input,
//     UnifiedCOOOutput& out)
// {
//     int threads = 128;
//     int blocks = (csf.nnz + threads - 1) / threads;
//
//     einsum_universal_leaf_kernel<<<blocks, threads>>>(
//         csf.d_vals,
//         d_dense_input,
//         csf.d_coordinate_matrix,
//         plan,
//         csf,
//         out.d_out_indices,
//         out.d_out_vals,
//         out.d_global_counter,
//         out.out_order
//     );
// }

#include "csf_einsum.cuh"

__global__ void generate_csf_ptrs_kernel(
    const int* __restrict__ coo_indices,
    int nnz,
    int out_order,
    int target_dim,
    int* __restrict__ csf_ptrs,
    int* __restrict__ csf_indices,
    int* __restrict__ unique_counter)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= nnz) return;

    bool is_boundary = (idx == 0);
    if (idx > 0) {
        for (int d = 0; d <= target_dim; ++d) {
            if (coo_indices[idx * out_order + d] != coo_indices[(idx - 1) * out_order + d]) {
                is_boundary = true;
                break;
            }
        }
    }

    if (is_boundary) {
        int slot = atomicAdd(unique_counter, 1);
        csf_indices[slot] = coo_indices[idx * out_order + target_dim];
        csf_ptrs[slot] = idx;
    }

    if (idx == nnz - 1) {
        int total_unique = *unique_counter;
        csf_ptrs[total_unique] = nnz;
    }
}

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
        int out_slot = atomicAdd(global_output_counter, 1);
        coo_out_vals[out_slot] = calcolo;

        int out_d = 0;
        for (int d = 0; d < csf.order; ++d) {
            bool is_contracted = false;
            for (int c = 0; c < plan.num_contracted_modes; ++c) {
                if (plan.contracted_levels[c] == d) { is_contracted = true; break; }
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
        csf.d_vals, d_dense_input, csf.d_coordinate_matrix,
        plan, csf, out.d_out_indices, out.d_out_vals, out.d_global_counter, out.out_order
    );
}

// =========================================================================
// TRASFORMAZIONE ZERO-ALLOCATION IN-PLACE: REUT L'INTERO BUFFER DI OUTPUT
// =========================================================================
UniversalCSF convert_coo_to_csf_on_gpu(UnifiedCOOOutput& coo_out) {
    UniversalCSF res_csf{};

    int h_nnz = 0;
    // Recuperiamo il contatore atomico per sapere quanti elementi validi ha scritto l'einsum
    cudaMemcpy(&h_nnz, coo_out.d_global_counter, sizeof(int), cudaMemcpyDeviceToHost);

    res_csf.nnz = h_nnz;
    res_csf.order = coo_out.out_order;
    res_csf.d_unified_buffer = coo_out.d_unified_buffer;
    res_csf.total_buffer_bytes = coo_out.total_buffer_bytes;

    for (int i = 0; i < MAX_TENSOR_ORDER; ++i) {
        res_csf.ptr_offsets[i] = 0;
        res_csf.ind_offsets[i] = 0;
    }

    if (h_nnz == 0) return res_csf;

    // Ripartizione geometrica in-place
    size_t indices_bytes = coo_out.max_nnz * coo_out.out_order * sizeof(int);
    char* d_byte_base = reinterpret_cast<char*>(coo_out.d_unified_buffer);

    // I valori finali vengono agganciati direttamente alla sezione originaria dei valori COO
    res_csf.d_vals = reinterpret_cast<float*>(d_byte_base + indices_bytes);

    // Siccome l'Einsum ha ridotto il dataset (h_nnz <= max_nnz), usiamo lo spazio extra
    // che avanza alla fine dell'array dei valori come Scratchpad interna per all_ptrs.
    // L'inizio dello spazio di manovra è posizionato dopo i reali float validi (h_nnz * sizeof(float))
    char* d_scratchpad_ptr = d_byte_base + indices_bytes + (h_nnz * sizeof(float));

    int* d_all_ptrs_base = reinterpret_cast<int*>(d_scratchpad_ptr);
    int* d_all_indices_base = coo_out.d_out_indices;

    int current_ptr_offset = 0;
    int current_ind_offset = 0;

    int threads = 128;
    int blocks = (h_nnz + threads - 1) / threads;

    // Sfruttiamo gli ultimissimi byte in fondo al buffer (dove c'era il d_global_counter di 4 byte)
    // come variabile atomica temporanea per il kernel di scansione, evitando QUALSIASI cudaMalloc extra.
    int* d_temp_level_counter = coo_out.d_global_counter;

    for (int d = 0; d < res_csf.order; ++d) {
        cudaMemset(d_temp_level_counter, 0, sizeof(int));

        res_csf.ptr_offsets[d] = current_ptr_offset;
        res_csf.ind_offsets[d] = current_ind_offset;

        generate_csf_ptrs_kernel<<<blocks, threads>>>(
            coo_out.d_out_indices, h_nnz, res_csf.order, d,
            d_all_ptrs_base + current_ptr_offset,
            d_all_indices_base + current_ind_offset,
            d_temp_level_counter
        );
        cudaDeviceSynchronize();

        int h_level_unique_nodes = 0;
        cudaMemcpy(&h_level_unique_nodes, d_temp_level_counter, sizeof(int), cudaMemcpyDeviceToHost);

        current_ptr_offset += (h_level_unique_nodes + 1);
        current_ind_offset += h_level_unique_nodes;
    }

    res_csf.d_coordinate_matrix = d_all_indices_base;
    res_csf.d_all_ptrs = d_all_ptrs_base;

    return res_csf;
}
