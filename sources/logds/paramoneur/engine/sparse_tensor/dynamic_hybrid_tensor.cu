#include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"
// #include <cuda_runtime.h>
// #include <device_launch_parameters.h>
// #include <thrust/device_ptr.h>
// #include <thrust/sort.h>
// #include <thrust/sequence.h>
// #include <thrust/binary_search.h>
// #include <thrust/execution_policy.h>
// // #include <cub/device/device_select.hpp>
// // #include <cub/device/device_reduce.hpp>
//
// // Kernel per generare i flag di segmentazione (rileva quando cambia un nodo genitore)
// __global__ void generate_parent_flags_kernel(
//     const IndexTuple* __restrict__ d_coords, int nnz, int current_level,
//     int* __restrict__ d_flags)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= nnz) return;
//
//     if (idx == 0) {
//         d_flags[idx] = 1;
//         return;
//     }
//
//     int match = 1;
//     for (int d = 0; d <= current_level; ++d) {
//         if (d_coords[idx].idx[d] != d_coords[idx - 1].idx[d]) {
//             match = 0;
//             break;
//         }
//     }
//     d_flags[idx] = (match == 0) ? 1 : 0;
// }
//
// // Kernel per estrarre la coordinata reale f_crd per i nodi unici scoperti
// __global__ void populate_fiber_coordinates_kernel(
//     const IndexTuple* __restrict__ d_coords, const int* __restrict__ d_unique_indices,
//     int num_unique_nodes, int current_level,
//     int* __restrict__ d_f_crd)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= num_unique_nodes) return;
//
//     int original_coo_idx = d_unique_indices[idx];
//     d_f_crd[idx] = d_coords[original_coo_idx].idx[current_level];
// }
//
// // Kernel per mappare le relazioni padre-figlio tramite ricerca binaria sulle tuple ordinate
// __global__ void map_parent_channels_kernel(
//     const int* __restrict__ d_unique_indices_child, int num_children,
//     const int* __restrict__ d_unique_indices_parent, int num_parents,
//     const IndexTuple* __restrict__ d_coords, int parent_level,
//     int* __restrict__ d_parent_ids)
// {
//     int child_idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (child_idx >= num_children) return;
//
//     int coo_idx = d_unique_indices_child[child_idx];
//
//     int low = 0;
//     int high = num_parents - 1;
//     int found_parent = 0;
//
//     while (low <= high) {
//         int mid = low + (high - low) / 2;
//         int parent_coo_idx = d_unique_indices_parent[mid];
//
//         int cmp = 0;
//         for (int d = 0; d <= parent_level; ++d) {
//             if (d_coords[coo_idx].idx[d] != d_coords[parent_coo_idx].idx[d]) {
//                 cmp = (d_coords[coo_idx].idx[d] < d_coords[parent_coo_idx].idx[d]) ? -1 : 1;
//                 break;
//             }
//         }
//
//         if (cmp == 0) {
//             found_parent = mid;
//             break;
//         } else if (cmp < 0) {
//             high = mid - 1;
//         } else {
//             low = mid + 1;
//         }
//     }
//     d_parent_ids[child_idx] = found_parent;
// }
//
// void
// map_parent_channels_kernel(const HostCOO& host_coo, CSFLevelDevice* h_levels, float*& d_final_vals) {
//     int nnz = host_coo.nnz;
//     int order = host_coo.order;
//
//     if (nnz == 0) return;
//
//     IndexTuple* d_coords;
//     cudaMalloc(&d_coords, nnz * sizeof(IndexTuple));
//     cudaMalloc(&d_final_vals, nnz * sizeof(float));
//
//     cudaMemcpy(d_coords, host_coo.coords.data(), nnz * sizeof(IndexTuple), cudaMemcpyHostToDevice);
//     cudaMemcpy(d_final_vals, host_coo.values.data(), nnz * sizeof(float), cudaMemcpyHostToDevice);
//
//     thrust::device_ptr<IndexTuple> dev_coords(d_coords);
//     thrust::device_ptr<float> dev_vals(d_final_vals);
//
//     LexicographicalLessThan comp(order);
//     thrust::sort_by_key(thrust::device, dev_coords, dev_coords + nnz, dev_vals, comp);
//
//     std::vector<int*> unique_indices_per_level(order);
//     std::vector<int> num_nodes_per_level(order);
//
//     int block = 256;
//     int grid = (nnz + block - 1) / block;
//
//     for (int d = 0; d < order; ++d) {
//         int* d_flags;
//         cudaMalloc(&d_flags, nnz * sizeof(int));
//         generate_parent_flags_kernel<<<grid, block>>>(d_coords, nnz, d, d_flags);
//
//         int* d_unique_indices;
//         cudaMalloc(&d_unique_indices, nnz * sizeof(int));
//         int* d_num_selected;
//         cudaMalloc(&d_num_selected, sizeof(int));
//
//         int* d_seq;
//         cudaMalloc(&d_seq, nnz * sizeof(int));
//         thrust::device_ptr<int> dev_seq(d_seq);
//         thrust::sequence(thrust::device, dev_seq, dev_seq + nnz);
//
//         void* d_temp_storage = nullptr;
//         size_t temp_storage_bytes = 0;
//         cub::DeviceSelect::Flagged(d_temp_storage, temp_storage_bytes, d_seq, d_flags, d_unique_indices, d_num_selected, nnz);
//         cudaMalloc(&d_temp_storage, temp_storage_bytes);
//         cub::DeviceSelect::Flagged(d_temp_storage, temp_storage_bytes, d_seq, d_flags, d_unique_indices, d_num_selected, nnz);
//
//         int h_num_nodes = 0;
//         cudaMemcpy(&h_num_nodes, d_num_selected, sizeof(int), cudaMemcpyDeviceToHost);
//         num_nodes_per_level[d] = h_num_nodes;
//         unique_indices_per_level[d] = d_unique_indices;
//
//         cudaMalloc(&h_levels[d].f_crd, h_num_nodes * sizeof(int));
//         h_levels[d].num_nodes = h_num_nodes;
//
//         int grid_nodes = (h_num_nodes + block - 1) / block;
//         populate_fiber_coordinates_kernel<<<grid_nodes, block>>>(d_coords, d_unique_indices, h_num_nodes, d, h_levels[d].f_crd);
//
//         cudaFree(d_flags);
//         cudaFree(d_num_selected);
//         cudaFree(d_temp_storage);
//         cudaFree(d_seq);
//     }
//
//     for (int d = 0; d < order - 1; ++d) {
//         int num_parents = num_nodes_per_level[d];
//         int num_children = num_nodes_per_level[d + 1];
//
//         int* d_parent_ids;
//         cudaMalloc(&d_parent_ids, num_children * sizeof(int));
//
//         int grid_children = (num_children + block - 1) / block;
//         map_parent_channels_kernel<<<grid_children, block>>>(
//             unique_indices_per_level[d + 1], num_children,
//             unique_indices_per_level[d], num_parents,
//             d_coords, d, d_parent_ids
//         );
//
//         cudaMalloc(&h_levels[d].f_ptr, (num_parents + 1) * sizeof(int));
//
//         thrust::counting_iterator<int> search_begin(0);
//         thrust::upper_bound(thrust::device, d_parent_ids, d_parent_ids + num_children, search_begin, search_begin + num_parents, h_levels[d].f_ptr + 1);
//
//         int zero = 0;
//         cudaMemcpy(h_levels[d].f_ptr, &zero, sizeof(int), cudaMemcpyHostToDevice);
//         // CORREZIONE CRITICA: Scrittura dell'offset di chiusura finale della fibra
//         cudaMemcpy(h_levels[d].f_ptr + num_parents, &num_children, sizeof(int), cudaMemcpyHostToDevice);
//
//         cudaFree(d_parent_ids);
//     }
//
//     h_levels[order - 1].f_ptr = nullptr;
//
//     for (int d = 0; d < order; ++d) {
//         cudaFree(unique_indices_per_level[d]);
//     }
//     cudaFree(d_coords);
// }

// #include "dynamic_hybrid_tensor.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/device_ptr.h>
#include <thrust/sort.h>
#include <thrust/sequence.h>
#include <thrust/binary_search.h>
#include <thrust/execution_policy.h>
// #include <cub/device/device_select.hpp>
// #include <cub/device/device_reduce.hpp>

// ============================================================================
// KERNEL PER LA COSTRUZIONE DELL'ALBERO CSF DA TUPLE COO ORDINATE
// ============================================================================


//
// __global__ void generate_parent_flags_kernel(
//     const IndexTuple* __restrict__ d_coords, int nnz, int current_level,
//     int* __restrict__ d_flags)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= nnz) return;
//
//     if (idx == 0) {
//         d_flags[idx] = 1;
//         return;
//     }
//
//     int match = 1;
//     for (int d = 0; d <= current_level; ++d) {
//         if (d_coords[idx].idx[d] != d_coords[idx - 1].idx[d]) {
//             match = 0;
//             break;
//         }
//     }
//     d_flags[idx] = (match == 0) ? 1 : 0;
// }
//
// __global__ void populate_fiber_coordinates_kernel(
//     const IndexTuple* __restrict__ d_coords, const int* __restrict__ d_unique_indices,
//     int num_unique_nodes, int current_level,
//     int* __restrict__ d_f_crd)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= num_unique_nodes) return;
//
//     int original_coo_idx = d_unique_indices[idx];
//     d_f_crd[idx] = d_coords[original_coo_idx].idx[current_level];
// }
//
// __global__ void map_parent_channels_kernel(
//     const int* __restrict__ d_unique_indices_child, int num_children,
//     const int* __restrict__ d_unique_indices_parent, int num_parents,
//     const IndexTuple* __restrict__ d_coords, int parent_level,
//     int* __restrict__ d_parent_ids)
// {
//     int child_idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (child_idx >= num_children) return;
//
//     int coo_idx = d_unique_indices_child[child_idx];
//
//     int low = 0;
//     int high = num_parents - 1;
//     int found_parent = 0;
//
//     while (low <= high) {
//         int mid = low + (high - low) / 2;
//         int parent_coo_idx = d_unique_indices_parent[mid];
//
//         int cmp = 0;
//         for (int d = 0; d <= parent_level; ++d) {
//             if (d_coords[coo_idx].idx[d] != d_coords[parent_coo_idx].idx[d]) {
//                 cmp = (d_coords[coo_idx].idx[d] < d_coords[parent_coo_idx].idx[d]) ? -1 : 1;
//                 break;
//             }
//         }
//
//         if (cmp == 0) {
//             found_parent = mid;
//             break;
//         } else if (cmp < 0) {
//             high = mid - 1;
//         } else {
//             low = mid + 1;
//         }
//     }
//     d_parent_ids[child_idx] = found_parent;
// }
//
// // ============================================================================
// // KERNEL DI CALCOLO EINSUM GENERICA (RICERCA ITERATIVA SU GRAFO CSF)
// // ============================================================================
//
// __global__ void csf_generic_einsum_kernel(
//     const CSFLevel* __restrict__ A_levels, const float* __restrict__ A_vals, int A_root_size,
//     const CSFLevel* __restrict__ B_levels, const float* __restrict__ B_vals, int B_root_size,
//     EinsumPlan plan,
//     COOElement* __restrict__ coo_workspace,
//     int* __restrict__ global_coo_counter,
//     int max_workspace_size)
// {
//     int root_worker_id = blockIdx.x * blockDim.x + threadIdx.x;
//
//     // Struttura dello stack locale allocato nei registri hardware del thread
//     int stack_start_A[MAX_ORDER * 2];
//     int stack_end_A[MAX_ORDER * 2];
//     int stack_start_B[MAX_ORDER * 2];
//     int stack_end_B[MAX_ORDER * 2];
//     int curr_p_A[MAX_ORDER * 2];
//     int curr_p_B[MAX_ORDER * 2];
//     int current_out_coords[MAX_ORDER];
//
//     #pragma unroll
//     for (int i = 0; i < MAX_ORDER; ++i) current_out_coords[i] = 0;
//
//     int current_axis_idx = 0;
//     const AxisStep first_step = plan.steps[0];
//
//     // Setup della radice (Asse Logico 0)
//     if (first_step.map_type == AXIS_IN_BOTH || first_step.map_type == AXIS_ONLY_A) {
//         if (root_worker_id >= A_root_size) return;
//         stack_start_A[0] = root_worker_id;
//         stack_end_A[0]   = root_worker_id + 1;
//         curr_p_A[0]      = root_worker_id;
//
//         if (first_step.map_type == AXIS_IN_BOTH) {
//             stack_start_B[0] = 0;
//             stack_end_B[0]   = B_root_size;
//             curr_p_B[0]      = 0;
//         }
//     } else {
//         if (root_worker_id >= B_root_size) return;
//         stack_start_B[0] = root_worker_id;
//         stack_end_B[0]   = root_worker_id + 1;
//         curr_p_B[0]      = root_worker_id;
//
//         stack_start_A[0] = 0;
//         stack_end_A[0]   = 0;
//         curr_p_A[0]      = 0;
//     }
//
//     // Algoritmo di Backtracking Esplicito
//     while (current_axis_idx >= 0) {
//         const AxisStep step = plan.steps[current_axis_idx];
//         bool match_found = false;
//
//         if (step.map_type == AXIS_IN_BOTH) {
//             int pA = curr_p_A[current_axis_idx];
//             int pB = curr_p_B[current_axis_idx];
//             int endA = stack_end_A[current_axis_idx];
//             int endB = stack_end_B[current_axis_idx];
//
//             while (pA < endA && pB < endB) {
//                 int crd_A = A_levels[step.level_in_A].f_crd[pA];
//                 int crd_B = B_levels[step.level_in_B].f_crd[pB];
//
//                 if (crd_A == crd_B) {
//                     curr_p_A[current_axis_idx] = pA;
//                     curr_p_B[current_axis_idx] = pB;
//
//                     if (step.out_axis_pos != -1) {
//                         current_out_coords[step.out_axis_pos] = crd_A;
//                     }
//                     match_found = true;
//                     break;
//                 } else if (crd_A < crd_B) {
//                     pA++;
//                 } else {
//                     pB++;
//                 }
//             }
//             if (!match_found) {
//                 curr_p_A[current_axis_idx] = pA;
//                 curr_p_B[current_axis_idx] = pB;
//             }
//         }
//         else if (step.map_type == AXIS_ONLY_A) {
//             int pA = curr_p_A[current_axis_idx];
//             if (pA < stack_end_A[current_axis_idx]) {
//                 int crd_A = A_levels[step.level_in_A].f_crd[pA];
//                 if (step.out_axis_pos != -1) {
//                     current_out_coords[step.out_axis_pos] = crd_A;
//                 }
//                 match_found = true;
//             }
//         }
//         else if (step.map_type == AXIS_ONLY_B) {
//             int pB = curr_p_B[current_axis_idx];
//             if (pB < stack_end_B[current_axis_idx]) {
//                 int crd_B = B_levels[step.level_in_B].f_crd[pB];
//                 if (step.out_axis_pos != -1) {
//                     current_out_coords[step.out_axis_pos] = crd_B;
//                 }
//                 match_found = true;
//             }
//         }
//
//         if (match_found) {
//             if (current_axis_idx == plan.total_logical_axes - 1) {
//                 float val_A = (step.level_in_A != -1) ? A_vals[curr_p_A[current_axis_idx]] : 1.0f;
//                 float val_B = (step.level_in_B != -1) ? B_vals[curr_p_B[current_axis_idx]] : 1.0f;
//                 float out_val = val_A * val_B;
//
//                 if (out_val != 0.0f) {
//                     int ticket = atomicAdd(global_coo_counter, 1);
//                     if (ticket < max_workspace_size) {
//                         coo_workspace[ticket].value = out_val;
//                         #pragma unroll
//                         for (int i = 0; i < MAX_ORDER; ++i) {
//                             coo_workspace[ticket].coords[i] = (i < plan.out_order) ? current_out_coords[i] : 0;
//                         }
//                     }
//                 }
//
//                 if (step.map_type == AXIS_IN_BOTH) {
//                     curr_p_A[current_axis_idx]++;
//                     curr_p_B[current_axis_idx]++;
//                 } else if (step.map_type == AXIS_ONLY_A) {
//                     curr_p_A[current_axis_idx]++;
//                 } else {
//                     curr_p_B[current_axis_idx]++;
//                 }
//             }
//             else {
//                 int next_axis = current_axis_idx + 1;
//                 const AxisStep next_step = plan.steps[next_axis];
//
//                 if (next_step.level_in_A != -1 && (step.map_type == AXIS_IN_BOTH || step.map_type == AXIS_ONLY_A)) {
//                     int pA = curr_p_A[current_axis_idx];
//                     stack_start_A[next_axis] = A_levels[step.level_in_A].f_ptr[pA];
//                     stack_end_A[next_axis]   = A_levels[step.level_in_A].f_ptr[pA + 1];
//                     curr_p_A[next_axis]      = stack_start_A[next_axis];
//                 }
//
//                 if (next_step.level_in_B != -1 && (step.map_type == AXIS_IN_BOTH || step.map_type == AXIS_ONLY_B)) {
//                     int pB = curr_p_B[current_axis_idx];
//                     stack_start_B[next_axis] = B_levels[step.level_in_B].f_ptr[pB];
//                     stack_end_B[next_axis]   = B_levels[step.level_in_B].f_ptr[pB + 1];
//                     curr_p_B[next_axis]      = stack_start_B[next_axis];
//                 }
//
//                 current_axis_idx = next_axis;
//             }
//         }
//         else {
//             current_axis_idx--;
//             if (current_axis_idx >= 0) {
//                 const AxisStep prev_step = plan.steps[current_axis_idx];
//                 if (prev_step.map_type == AXIS_IN_BOTH) {
//                     curr_p_A[current_axis_idx]++;
//                     curr_p_B[current_axis_idx]++;
//                 } else if (prev_step.map_type == AXIS_ONLY_A) {
//                     curr_p_A[current_axis_idx]++;
//                 } else {
//                     curr_p_B[current_axis_idx]++;
//                 }
//             }
//         }
//     }
// }
//
// // Comparatore per l'ordinamento lessicografico sul device hardware
// struct LexicographicalLessThan {
//     int order;
//     __host__ __device__ LexicographicalLessThan(int o) : order(o) {}
//     __host__ __device__ bool operator()(const IndexTuple& a, const IndexTuple& b) const {
//         for (int i = 0; i < order; ++i) {
//             if (a.idx[i] < b.idx[i]) return true;
//             if (a.idx[i] > b.idx[i]) return false;
//         }
//         return false; // Elementi identici
//     }
// };
//
// void load_coo_host_to_csf_device(const HostCOO& host_coo, CSFLevelDevice* h_levels, float*& d_final_vals) {
//     int nnz = host_coo.nnz; int order = host_coo.order;
//     if (nnz == 0) return;
//
//     IndexTuple* d_coords;
//     cudaMalloc(&d_coords, nnz * sizeof(IndexTuple));
//     cudaMalloc(&d_final_vals, nnz * sizeof(float));
//     cudaMemcpy(d_coords, host_coo.coords.data(), nnz * sizeof(IndexTuple), cudaMemcpyHostToDevice);
//     cudaMemcpy(d_final_vals, host_coo.values.data(), nnz * sizeof(float), cudaMemcpyHostToDevice);
//
//     thrust::device_ptr<IndexTuple> dev_coords(d_coords);
//     thrust::device_ptr<float> dev_vals(d_final_vals);
//     LexicographicalLessThan comp(order);
//     thrust::sort_by_key(thrust::device, dev_coords, dev_coords + nnz, dev_vals, comp);
//
//     std::vector<int*> unique_indices_per_level(order);
//     std::vector<int> num_nodes_per_level(order);
//     int block = 256; int grid = (nnz + block - 1) / block;
//
//     for (int d = 0; d < order; ++d) {
//         int* d_flags; cudaMalloc(&d_flags, nnz * sizeof(int));
//         generate_parent_flags_kernel<<<grid, block>>>(d_coords, nnz, d, d_flags);
//
//         int* d_unique_indices; cudaMalloc(&d_unique_indices, nnz * sizeof(int));
//         int* d_num_selected; cudaMalloc(&d_num_selected, sizeof(int));
//         int* d_seq; cudaMalloc(&d_seq, nnz * sizeof(int));
//         thrust::device_ptr<int> dev_seq(d_seq); thrust::sequence(thrust::device, dev_seq, dev_seq + nnz);
//
//         void* d_temp_storage = nullptr; size_t temp_storage_bytes = 0;
//         cub::DeviceSelect::Flagged(d_temp_storage, temp_storage_bytes, d_seq, d_flags, d_unique_indices, d_num_selected, nnz);
//         cudaMalloc(&d_temp_storage, temp_storage_bytes);
//         cub::DeviceSelect::Flagged(d_temp_storage, temp_storage_bytes, d_seq, d_flags, d_unique_indices, d_num_selected, nnz);
//
//         int h_num_nodes = 0;   cudaMemcpy(&h_num_nodes, d_num_selected, sizeof(int), cudaMemcpyDeviceToHost);
//         num_nodes_per_level[d] = h_num_nodes; unique_indices_per_level[d] = d_unique_indices;
//
//         cudaMalloc(&h_levels[d].f_crd, h_num_nodes * sizeof(int)); h_levels[d].num_nodes = h_num_nodes;
//         int grid_nodes = (h_num_nodes + block - 1) / block;
//         populate_fiber_coordinates_kernel<<<grid_nodes, block>>>(d_coords, d_unique_indices, h_num_nodes, d, h_levels[d].f_crd);
//
//         cudaFree(d_flags); cudaFree(d_num_selected); cudaFree(d_temp_storage); cudaFree(d_seq);
//     }
//
//     for (int d = 0; d < order - 1; ++d) {
//         int num_parents = num_nodes_per_level[d]; int num_children = num_nodes_per_level[d + 1];
//         int* d_parent_ids; cudaMalloc(&d_parent_ids, num_children * sizeof(int));
//         int grid_children = (num_children + block - 1) / block;
//         map_parent_channels_kernel<<<grid_children, block>>>(unique_indices_per_level[d + 1], num_children, unique_indices_per_level[d], num_parents, d_coords, d, d_parent_ids);
//
//         cudaMalloc(&h_levels[d].f_ptr, (num_parents + 1) * sizeof(int));
//         thrust::counting_iterator<int> search_begin(0);
//         thrust::upper_bound(thrust::device, d_parent_ids, d_parent_ids + num_children, search_begin, search_begin + num_parents, h_levels[d].f_ptr + 1);
//
//         int zero = 0; cudaMemcpy(h_levels[d].f_ptr, &zero, sizeof(int), cudaMemcpyHostToDevice);
//         cudaMemcpy(h_levels[d].f_ptr + num_parents, &num_children, sizeof(int), cudaMemcpyHostToDevice);
//         cudaFree(d_parent_ids);
//     }
//     h_levels[order - 1].f_ptr = nullptr;
//     for (int d = 0; d < order; ++d) cudaFree(unique_indices_per_level[d]);
//     cudaFree(d_coords);
// }
//
// void launch_csf_einsum_wrapper(
// const CSFLevel* d_A_levels, const float* d_vals_A, int A_root_size,
// const CSFLevel* d_B_levels, const float* d_vals_B, int B_root_size,
// EinsumPlan plan, COOElement* d_coo_workspace, int* d_global_coo_counter, int max_workspace_size)
// {
// int threads = 64; int blocks = (A_root_size + threads - 1) / threads;
// if (blocks == 0) blocks = 1;
// csf_generic_einsum_kernel<<<blocks, threads>>>(d_A_levels, d_vals_A, A_root_size, d_B_levels, d_vals_B, B_root_size, plan, d_coo_workspace, d_global_coo_counter, max_workspace_size);
// }