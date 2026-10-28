// /*
// #include <cuda_runtime.h>
// #include <device_launch_parameters.h>
// #include <cstdint>
//
// #include <cuda_runtime.h>
// #include <device_launch_parameters.h>
// #include <cstdint>
//
// #include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"
//
//
// // Inserimento atomico asincrono nel Delta Buffer per Adam (O(1))
// __global__ void delta_insert_kernel(uint64_t** delta_coords, float* delta_vals, uint32_t* delta_nnz,
//                                     size_t num_dims, const uint64_t* coords, float value, size_t capacity) {
//     if (blockIdx.x == 0 && threadIdx.x == 0) {
//         uint32_t slot = atomicAdd(delta_nnz, 1);
//         if (slot >= capacity) return; // Protezione da overflow fisico
//
//         for (size_t d = 0; d < num_dims; ++d) {
//             delta_coords[d][slot] = coords[d];
//         }
//         delta_vals[slot] = value;
//     }
// }
//
// // Proiezione lineare multi-asse per l'aggregazione geometrica delle coordinate
// __global__ void mask_axes_kernel(const uint64_t* src_keys, uint64_t* dst_keys, size_t nnz,
//                                   const uint64_t* strides, const bool* keep_axes, size_t num_dims) {
//     size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= nnz) return;
//
//     uint64_t key = src_keys[idx];
//     uint64_t rem = key;
//     uint64_t coords[7]; // Stack statico sicuro per max 7 dimensioni
//
//     for (size_t i = 0; i < num_dims; ++i) {
//         coords[i] = rem / strides[i];
//         rem = rem % strides[i];
//     }
//
//     uint64_t new_key = 0;
//     uint64_t current_stride = 1;
//     for (int i = static_cast<int>(num_dims) - 1; i >= 0; --i) {
//         if (keep_axes[i]) {
//             new_key += coords[i] * current_stride;
//             current_stride *= (coords[i] + 1);
//         }
//     }
//     dst_keys[idx] = new_key;
// }
//
// #include <cuda_runtime.h>
// #include <cstdint>
//
// __global__ void einstein_contraction_binary_kernel(
//     const float* vals_A, uint64_t** coords_A, size_t nnz_A,
//     const float* vals_B, uint64_t** coords_B, size_t nnz_B,
//     float* vals_Out, uint64_t** coords_Out, uint32_t* out_nnz,
//     const int* map_A, const int* map_B, size_t total_unique_indices,
//     const int* out_axes_map, size_t out_dims)
// {
//     size_t idx_A = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx_A >= nnz_A) return;
//
//     // Ogni thread dell'accumulatore A scansiona gli elementi di B alla ricerca di un match geometrico
//     for (size_t idx_B = 0; idx_B < nnz_B; ++idx_B) {
//         bool match = true;
//
//         // Controlla l'uguaglianza geometrica solo sugli assi contratti (che hanno ID comuni)
//         for (size_t i = 0; i < total_unique_indices; ++i) {
//             int axis_A = map_A[i];
//             int axis_B = map_B[i];
//             if (axis_A >= 0 && axis_B >= 0) {
//                 if (coords_A[axis_A][idx_A] != coords_B[axis_B][idx_B]) {
//                     match = false;
//                     break;
//                 }
//             }
//         }
//
//         if (match) {
//             // Assegnazione atomica dello slot di output (Non-deterministica a livello hardware)
//             uint32_t out_slot = atomicAdd(out_nnz, 1);
//
//             // Scrittura del valore contratto
//             vals_Out[out_slot] = vals_A[idx_A] * vals_B[idx_B];
//
//             // Mappatura delle coordinate di output in base alla ricetta
//             for (size_t d = 0; d < out_dims; ++d) {
//                 int global_axis_id = out_axes_map[d];
//                 int src_axis_A = map_A[global_axis_id];
//                 int src_axis_B = map_B[global_axis_id];
//
//                 if (src_axis_A >= 0) {
//                     coords_Out[d][out_slot] = coords_A[src_axis_A][idx_A];
//                 } else if (src_axis_B >= 0) {
//                     coords_Out[d][out_slot] = coords_B[src_axis_B][idx_B];
//                 }
//             }
//         }
//     }
// }
//
//
// // Kernel di accoppiamento per Hadamard punto a punto (A = A * B)
// __global__ void coo_hadamard_kernel(const uint64_t* keys_A, float* vals_A, size_t nnz_A,
//                                     const uint64_t* keys_B, const float* vals_B, size_t nnz_B) {
//     size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= nnz_A) return;
//
//     uint64_t key_A = keys_A[idx];
//     size_t low = 0, high = nnz_B;
//     bool found = false;
//
//     while (low < high) {
//         size_t mid = low + (high - low) / 2;
//         if (keys_B[mid] == key_A) {
//             vals_A[idx] *= vals_B[mid];
//             found = true;
//             break;
//         } else if (keys_B[mid] < key_A) { low = mid + 1; }
//         else { high = mid; }
//     }
//     if (!found) vals_A[idx] = 0.0f; // Sfoltimento logico in-place
// }
//
// // Kernel Element-wise Add per la somma delle celle (A = A + B)
// __global__ void coo_cell_sum_kernel(float* vals_A, const float* vals_B, size_t nnz) {
//     size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx < nnz) {
//         vals_A[idx] += vals_B[idx];
//     }
// }
//
// #else
//
//
//
//
// // Kernel per generare i flag di segmentazione (rileva quando cambia un nodo genitore)
// __global__ void generate_parent_flags_kernel(
//     const IndexTuple* __restrict__ d_coords, int nnz, int current_level,
//     int* __restrict__ d_flags);
// // {
// //     int idx = blockIdx.x * blockDim.x + threadIdx.x;
// //     if (idx >= nnz) return;
// //
// //     if (idx == 0) {
// //         d_flags[idx] = 1;
// //         return;
// //     }
// //
// //     // Due elementi appartengono allo stesso nodo se tutte le loro coordinate
// //     // dal livello 0 fino a 'current_level' sono identiche
// //     int match = 1;
// //     for (int d = 0; d <= current_level; ++d) {
// //         if (d_coords[idx].idx[d] != d_coords[idx - 1].idx[d]) {
// //             match = 0;
// //             break;
// //         }
// //     }
// //     d_flags[idx] = (match == 0) ? 1 : 0;
// // }
//
// // Kernel per estrarre la coordinata reale f_crd per i nodi unici scoperti
// __global__ void populate_fiber_coordinates_kernel(
//     const IndexTuple* __restrict__ d_coords, const int* __restrict__ d_unique_indices,
//     int num_unique_nodes, int current_level,
//     int* __restrict__ d_f_crd);
// // {
// //     int idx = blockIdx.x * blockDim.x + threadIdx.x;
// //     if (idx >= num_unique_nodes) return;
// //
// //     int original_coo_idx = d_unique_indices[idx];
// //     d_f_crd[idx] = d_coords[original_coo_idx].idx[current_level];
// // }
//
// // Kernel per mappare le relazioni padre-figlio tramite ricerca binaria sulle tuple ordinate
// __global__ void map_parent_channels_kernel(
//     const int* __restrict__ d_unique_indices_child, int num_children,
//     const int* __restrict__ d_unique_indices_parent, int num_parents,
//     const IndexTuple* __restrict__ d_coords, int parent_level,
//     int* __restrict__ d_parent_ids);
// // {
// //     int child_idx = blockIdx.x * blockDim.x + threadIdx.x;
// //     if (child_idx >= num_children) return;
// //
// //     int coo_idx = d_unique_indices_child[child_idx];
// //
// //     // Ricerca binaria nell'array dei genitori per trovare la corrispondenza strutturale
// //     int low = 0;
// //     int high = num_parents - 1;
// //     int found_parent = 0;
// //
// //     while (low <= high) {
// //         int mid = low + (high - low) / 2;
// //         int parent_coo_idx = d_unique_indices_parent[mid];
// //
// //         int cmp = 0;
// //         for (int d = 0; d <= parent_level; ++d) {
// //             if (d_coords[coo_idx].idx[d] != d_coords[parent_coo_idx].idx[d]) {
// //                 cmp = (d_coords[coo_idx].idx[d] < d_coords[parent_coo_idx].idx[d]) ? -1 : 1;
// //                 break;
// //             }
// //         }
// //
// //         if (cmp == 0) {
// //             found_parent = mid;
// //             break;
// //         } else if (cmp < 0) {
// //             high = mid - 1;
// //         } else {
// //             low = mid + 1;
// //         }
// //     }
// //     d_parent_ids[child_idx] = found_parent;
// // }
//
//
//
// __global__ void csf_generic_einsum_kernel(
//     const CSFLevel* __restrict__ A_levels, const float* __restrict__ A_vals, int A_root_size,
//     const CSFLevel* __restrict__ B_levels, const float* __restrict__ B_vals, int B_root_size,
//     EinsumPlan plan,
//     COOElement* __restrict__ coo_workspace,
//     int* __restrict__ global_coo_counter,
//     int max_workspace_size)
// {
//     // // Identificatore globale del thread per la parallelizzazione delle radici
//     // int root_worker_id = blockIdx.x * blockDim.x + threadIdx.x;
//     //
//     // // Stack locali nei registri per tracciare gli indici dei nodi correnti lungo l'albero
//     // int stack_start_A[MAX_ORDER * 2];
//     // int stack_end_A[MAX_ORDER * 2];
//     // int stack_start_B[MAX_ORDER * 2];
//     // int stack_end_B[MAX_ORDER * 2];
//     //
//     // // Puntatori attuali all'interno dei nodi figli
//     // int curr_p_A[MAX_ORDER * 2];
//     // int curr_p_B[MAX_ORDER * 2];
//     //
//     // // Accumulatore delle coordinate parziali dell'output
//     // int current_out_coords[MAX_ORDER];
//     //
//     // int current_axis_idx = 0;
//     //
//     // // Inizializzazione del livello radice (Asse Logico 0)
//     // const AxisStep first_step = plan.steps[0];
//     //
//     // if (first_step.map_type == AXIS_IN_BOTH || first_step.map_type == AXIS_ONLY_A) {
//     //     if (root_worker_id >= A_root_size) return;
//     //     stack_start_A[0] = root_worker_id;
//     //     stack_end_A[0]   = root_worker_id + 1;
//     //     curr_p_A[0]      = root_worker_id;
//     //
//     //     if (first_step.map_type == AXIS_IN_BOTH) {
//     //         stack_start_B[0] = 0;
//     //         stack_end_B[0]   = B_root_size;
//     //         curr_p_B[0]      = 0; // Verrà cercato tramite matching coordinate
//     //     }
//     // } else { // AXIS_ONLY_B
//     //     if (root_worker_id >= B_root_size) return;
//     //     stack_start_B[0] = root_worker_id;
//     //     stack_end_B[0]   = root_worker_id + 1;
//     //     curr_p_B[0]      = root_worker_id;
//     // }
//     __global__ void csf_generic_einsum_kernel(
//         const CSFLevel* __restrict__ A_levels, const float* __restrict__ A_vals, int A_root_size,
//         const CSFLevel* __restrict__ B_levels, const float* __restrict__ B_vals, int B_root_size,
//         EinsumPlan plan,
//         COOElement* __restrict__ coo_workspace,
//         int* __restrict__ global_coo_counter,
//         int max_workspace_size)
//     {
//         int root_worker_id = blockIdx.x * blockDim.x + threadIdx.x;
//
//         int stack_start_A[MAX_ORDER * 2];
//         int stack_end_A[MAX_ORDER * 2];
//         int stack_start_B[MAX_ORDER * 2];
//         int stack_end_B[MAX_ORDER * 2];
//         int curr_p_A[MAX_ORDER * 2];
//         int curr_p_B[MAX_ORDER * 2];
//         int current_out_coords[MAX_ORDER];
//
//         // Inizializzazione accumulatori coordinate di sicurezza
//         for(int i = 0; i < MAX_ORDER; ++i) current_out_coords[i] = 0;
//
//         int current_axis_idx = 0;
//         const AxisStep first_step = plan.steps[0];
//
//         // CORREZIONE LOGICA: Impostazione accurata dei puntatori iniziali della radice
//         if (first_step.map_type == AXIS_IN_BOTH || first_step.map_type == AXIS_ONLY_A) {
//             if (root_worker_id >= A_root_size) return;
//
//             // Per il livello 0, i limiti coincidono con la posizione stessa del nodo unico
//             stack_start_A[0] = root_worker_id;
//             stack_end_A[0]   = root_worker_id + 1;
//             curr_p_A[0]      = root_worker_id;
//
//             if (first_step.map_type == AXIS_IN_BOTH) {
//                 stack_start_B[0] = 0;
//                 stack_end_B[0]   = B_root_size;
//                 curr_p_B[0]      = 0;
//             }
//         } else { // AXIS_ONLY_B
//             if (root_worker_id >= B_root_size) return;
//             stack_start_B[0] = root_worker_id;
//             stack_end_B[0]   = root_worker_id + 1;
//             curr_p_B[0]      = root_worker_id;
//
//             stack_start_A[0] = 0;
//             stack_end_A[0]   = 0;
//             curr_p_A[0]      = 0;
//         }
//
//     // Loop di backtracking iterativo per l'esplorazione dell'albero CSF
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
//             // Ricerca dell'intersezione delle coordinate (Two-Pointer Scan)
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
//             // Salviamo lo stato del puntatore per i cicli successivi su questo livello
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
//             // Se abbiamo completato l'intersezione di tutti gli assi logici, siamo alla foglia
//             if (current_axis_idx == plan.total_logical_axes - 1) {
//                 // Calcolo del valore finale recuperando i dati dagli array dei valori non nulli
//                 float val_A = 1.0f;
//                 float val_B = 1.0f;
//
//                 if (step.level_in_A != -1) {
//                     // Nel formato CSF le foglie dell'ultimo livello puntano all'indice di A_vals
//                     val_A = A_vals[curr_p_A[current_axis_idx]];
//                 }
//                 if (step.level_in_B != -1) {
//                     val_B = B_vals[curr_p_B[current_axis_idx]];
//                 }
//
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
//                 // Avanziamo il puntatore corrente per consumare l'elemento trovato e forzare la ricerca successiva
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
//                 // Configurazione ed espansione dei puntatori per l'asse successivo (Discesa nell'albero)
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
//             // Se non ci sono più match su questo livello, eseguiamo il Backtracking (Risalita)
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
// // #include <cub/cub.hpp>
// #include <thrust/device_ptr.h>
// #include <thrust/sort.h>
// #include <thrust/execution_policy.h>
//
// // Kernel di linearizzazione spaziale delle coordinate di output
// __global__ void linearize_coo_keys_kernel(
//     const COOElement* __restrict__ coo_workspace,
//     uint64_t* __restrict__ out_keys,
//     float* __restrict__ out_vals,
//     int nnz, int out_order, const int* __restrict__ out_dims)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= nnz) return;
//
//     uint64_t linear_key = 0;
//     uint64_t multiplier = 1;
//
//     // Linearizzazione partendo dall'ultima dimensione per preservare il Row-Major
//     for (int i = out_order - 1; i >= 0; --i) {
//         linear_key += coo_workspace[idx].coords[i] * multiplier;
//         multiplier *= out_dims[i];
//     }
//
//     out_keys[idx] = linear_key;
//     out_vals[idx] = coo_workspace[idx].value;
// }
//
// // Generatore del vettore finale row_ptr del CSR partendo dalle chiavi linearizzate condensate
// __global__ void generate_csr_row_ptr_kernel(
//     const uint64_t* __restrict__ unique_keys,
//     int* __restrict__ csr_row_ptr,
//     int unique_nnz, int num_rows, int num_cols)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= unique_nnz) return;
//
//     uint64_t current_row = unique_keys[idx] / num_cols;
//
//     // Rileva se siamo al confine della riga rispetto all'elemento precedente
//     if (idx == 0) {
//         for (int r = 0; r <= current_row; ++r) {
//             csr_row_ptr[r] = 0;
//         }
//     } else {
//         uint64_t prev_row = unique_keys[idx - 1] / num_cols;
//         if (current_row != prev_row) {
//             for (int r = prev_row + 1; r <= current_row; ++r) {
//                 csr_row_ptr[r] = idx;
//             }
//         }
//     }
//
//     // Scrittura del delimitatore finale per l'ultimo blocco
//     if (idx == unique_nnz - 1) {
//         for (int r = current_row + 1; r <= num_rows; ++r) {
//             csr_row_ptr[r] = unique_nnz;
//         }
//     }
// }
// //
// // void finalize_einsum_to_csr(
// //     COOElement* d_coo_workspace, int nnz_generated,
// //     int out_rows, int out_cols, int* d_out_dims,
// //     int*& d_csr_row_ptr, int*& d_csr_col_ind, float*& d_csr_vals, int& final_nnz)
// // {
// //     // 1. Allocazione array temporanei per chiavi e valori grezzi
// //     uint64_t* d_keys_raw;
// //     float* d_vals_raw;
// //     cudaMalloc(&d_keys_raw, nnz_generated * sizeof(uint64_t));
// //     cudaMalloc(&d_vals_raw, nnz_generated * sizeof(float));
// //
// //     int block = 256;
// //     int grid = (nnz_generated + block - 1) / block;
// //     linearize_coo_keys_kernel<<<grid, block>>>(d_coo_workspace, d_keys_raw, d_vals_raw, nnz_generated, 2, d_out_dims);
// //
// //     // 2. Ordinamento parallelo ad alte prestazioni via Thrust/CUB
// //     thrust::device_ptr<uint64_t> dev_keys(d_keys_raw);
// //     thrust::device_ptr<float> dev_vals(d_vals_raw);
// //     thrust::sort_by_key(thrust::device, dev_keys, dev_keys + nnz_generated, dev_vals);
// //
// //     // 3. Compattazione delle coordinate duplicate (Riduzione per chiave)
// //     uint64_t* d_unique_keys;
// //     float* d_unique_vals;
// //     cudaMalloc(&d_unique_keys, nnz_generated * sizeof(uint64_t));
// //     cudaMalloc(&d_unique_vals, nnz_generated * sizeof(float));
// //
// //     int* d_num_runs_out;
// //     cudaMalloc(&d_num_runs_out, sizeof(int));
// //
// //     // Determina lo spazio temporaneo richiesto da CUB per DeviceReduce::ReduceByKey
// //     void* d_temp_storage = nullptr;
// //     size_t temp_storage_bytes = 0;
// //     cub::DeviceReduce::ReduceByKey(
// //         d_temp_storage, temp_storage_bytes,
// //         d_keys_raw, d_unique_keys, d_vals_raw, d_unique_vals,
// //         d_num_runs_out, cub::Sum(), nnz_generated
// //     );
// //     cudaMalloc(&d_temp_storage, temp_storage_bytes);
// //
// //     // Esecuzione effettiva della riduzione
// //     cub::DeviceReduce::ReduceByKey(
// //         d_temp_storage, temp_storage_bytes,
// //         d_keys_raw, d_unique_keys, d_vals_raw, d_unique_vals,
// //         d_num_runs_out, cub::Sum(), nnz_generated
// //     );
// //
// //     cudaMemcpy(&final_nnz, d_num_runs_out, sizeof(int), cudaMemcpyDeviceToHost);
// //
// //     // 4. Generazione formati definitivi CSR
// //     cudaMalloc(&d_csr_row_ptr, (out_rows + 1) * sizeof(int));
// //     cudaMalloc(&d_csr_col_ind, final_nnz * sizeof(int));
// //     d_csr_vals = d_unique_vals; // Ricicliamo il puntatore ridotto
// //
// //     // Kernel per estrarre le colonne e ricostruire row_ptr
// //     int grid_final = (final_nnz + block - 1) / block;
// //     generate_csr_row_ptr_kernel<<<grid_final, block>>>(d_unique_keys, d_csr_row_ptr, final_nnz, out_rows, out_cols);
// //
// //     // Kernel inline per estrarre i col_ind
// //     thrust::transform(thrust::device, d_unique_keys, d_unique_keys + final_nnz, d_csr_col_ind,
// //         [=]  (uint64_t key) { return static_cast<int>(key % out_cols); });
// //
// //     // Pulizia
// //     cudaFree(d_keys_raw);
// //     cudaFree(d_vals_raw);
// //     cudaFree(d_unique_keys);
// //     cudaFree(d_temp_storage);
// //     cudaFree(d_num_runs_out);
// // }
//
// // Sostituto nativo di thrust::transform per l'estrazione degli indici di colonna
// __global__ void extract_csr_col_ind_kernel(
//     const uint64_t* __restrict__ unique_keys,
//     int* __restrict__ csr_col_ind,
//     int unique_nnz, int num_cols)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= unique_nnz) return;
//
//     csr_col_ind[idx] = static_cast<int>(unique_keys[idx] % num_cols);
// }
//
// void finalize_einsum_to_csr(
//     COOElement* d_coo_workspace, int nnz_generated,
//     int out_rows, int out_cols, int* d_out_dims,
//     int*& d_csr_row_ptr, int*& d_csr_col_ind, float*& d_csr_vals, int& final_nnz)
// {
//     if (nnz_generated == 0) {
//         final_nnz = 0;
//         cudaMalloc(&d_csr_row_ptr, (out_rows + 1) * sizeof(int));
//         cudaMemset(d_csr_row_ptr, 0, (out_rows + 1) * sizeof(int));
//         return;
//     }
//
//     uint64_t* d_keys_raw;
//     float* d_vals_raw;
//     cudaMalloc(&d_keys_raw, nnz_generated * sizeof(uint64_t));
//     cudaMalloc(&d_vals_raw, nnz_generated * sizeof(float));
//
//     int block = 256;
//     int grid = (nnz_generated + block - 1) / block;
//
//     // Chiamata interna al file .cu ammessa
//     linearize_coo_keys_kernel<<<grid, block>>>(d_coo_workspace, d_keys_raw, d_vals_raw, nnz_generated, 2, d_out_dims);
//
//     thrust::device_ptr<uint64_t> dev_keys(d_keys_raw);
//     thrust::device_ptr<float> dev_vals(d_vals_raw);
//     thrust::sort_by_key(thrust::device, dev_keys, dev_keys + nnz_generated, dev_vals);
//
//     uint64_t* d_unique_keys;
//     float* d_unique_vals;
//     cudaMalloc(&d_unique_keys, nnz_generated * sizeof(uint64_t));
//     cudaMalloc(&d_unique_vals, nnz_generated * sizeof(float));
//
//     int* d_num_runs_out;
//     cudaMalloc(&d_num_runs_out, sizeof(int));
//
//     void* d_temp_storage = nullptr;
//     size_t temp_storage_bytes = 0;
//     cub::DeviceReduce::ReduceByKey(
//         d_temp_storage, temp_storage_bytes,
//         d_keys_raw, d_unique_keys, d_vals_raw, d_unique_vals,
//         d_num_runs_out, cub::Sum(), nnz_generated
//     );
//     cudaMalloc(&d_temp_storage, temp_storage_bytes);
//     cub::DeviceReduce::ReduceByKey(
//         d_temp_storage, temp_storage_bytes,
//         d_keys_raw, d_unique_keys, d_vals_raw, d_unique_vals,
//         d_num_runs_out, cub::Sum(), nnz_generated
//     );
//
//     cudaMemcpy(&final_nnz, d_num_runs_out, sizeof(int), cudaMemcpyDeviceToHost);
//
//     cudaMalloc(&d_csr_row_ptr, (out_rows + 1) * sizeof(int));
//     cudaMalloc(&d_csr_col_ind, final_nnz * sizeof(int));
//     d_csr_vals = d_unique_vals;
//
//     int grid_final = (final_nnz + block - 1) / block;
//     generate_csr_row_ptr_kernel<<<grid_final, block>>>(d_unique_keys, d_csr_row_ptr, final_nnz, out_rows, out_cols);
//
//     // Lancio del kernel nativo sostitutivo di thrust::transform
//     extract_csr_col_ind_kernel<<<grid_final, block>>>(d_unique_keys, d_csr_col_ind, final_nnz, out_cols);
//
//     cudaFree(d_keys_raw);
//     cudaFree(d_vals_raw);
//     cudaFree(d_unique_keys);
//     cudaFree(d_temp_storage);
//     cudaFree(d_num_runs_out);
// }
//
// // --- IMPLEMENTAZIONE DEL WRAPPER HOST ---
// void launch_csf_einsum_wrapper(
//     const CSFLevel* d_A_levels, const float* d_vals_A, int A_root_size,
//     const CSFLevel* d_B_levels, const float* d_vals_B, int B_root_size,
//     EinsumPlan plan,
//     COOElement* d_coo_workspace,
//     int* d_global_coo_counter,
//     int max_workspace_size)
// {
//     // Configurazione dei blocchi e dei thread sul device hardware
//     int threads = 64;
//     int blocks = (A_root_size + threads - 1) / threads;
//
//     if (blocks == 0) blocks = 1; // Sicurezza per evitare lanci vuoti
//
//     // Chiamata nativa CUDA consentita solo dentro i file .cu
//     csf_generic_einsum_kernel<<<blocks, threads>>>(
//         d_A_levels, d_vals_A, A_root_size,
//         d_B_levels, d_vals_B, B_root_size,
//         plan, d_coo_workspace, d_global_coo_counter, max_workspace_size
//     );
//
//     // Opzionale: Controllo degli errori sincrono immediato post-lancio
// #ifdef DEBUG
//     cudaDeviceSynchronize();
//     cudaError_t err = cudaGetLastError();
//     if (err != cudaSuccess) {
//         printf("CUDA Error in Einsum Kernel: %s\n", cudaGetErrorString(err));
//     }
// #endif
// }*/
//
// #include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"
//
// #include <cuda_runtime.h>
// #include <device_launch_parameters.h>
// #include <thrust/device_ptr.h>
// #include <thrust/sort.h>
// #include <thrust/execution_policy.h>
// //#include <cub/device/device_reduce.hpp>
//
// // Kernel di linearizzazione spaziale delle coordinate multidimensionali di output in chiavi 1D
// __global__ void linearize_coo_keys_kernel(
//     const COOElement* __restrict__ coo_workspace,
//     uint64_t* __restrict__ out_keys,
//     float* __restrict__ out_vals,
//     int nnz, int out_order, const int* __restrict__ out_dims)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= nnz) return;
//
//     uint64_t linear_key = 0;
//     uint64_t multiplier = 1;
//
//     for (int i = out_order - 1; i >= 0; --i) {
//         linear_key += coo_workspace[idx].coords[i] * multiplier;
//         multiplier *= out_dims[i];
//     }
//
//     out_keys[idx] = linear_key;
//     out_vals[idx] = coo_workspace[idx].value;
// }
//
// // Generatore parallelo del vettore row_ptr del layout CSR basato sui confini delle chiavi
// __global__ void generate_csr_row_ptr_kernel(
//     const uint64_t* __restrict__ unique_keys,
//     int* __restrict__ csr_row_ptr,
//     int unique_nnz, int num_rows, int num_cols)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= unique_nnz) return;
//
//     uint64_t current_row = unique_keys[idx] / num_cols;
//
//     if (idx == 0) {
//         for (int r = 0; r <= current_row; ++r) csr_row_ptr[r] = 0;
//     } else {
//         uint64_t prev_row = unique_keys[idx - 1] / num_cols;
//         if (current_row != prev_row) {
//             for (int r = prev_row + 1; r <= current_row; ++r) {
//                 csr_row_ptr[r] = idx;
//             }
//         }
//     }
//
//     if (idx == unique_nnz - 1) {
//         for (int r = current_row + 1; r <= num_rows; ++r) {
//             csr_row_ptr[r] = unique_nnz;
//         }
//     }
// }
//
// // Kernel per estrarre l'indice di colonna (col_ind) tramite modulo della chiave lineare raggruppata
// __global__ void extract_csr_col_ind_kernel(
//     const uint64_t* __restrict__ unique_keys,
//     int* __restrict__ csr_col_ind,
//     int unique_nnz, int num_cols)
// {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx >= unique_nnz) return;
//
//     csr_col_ind[idx] = static_cast<int>(unique_keys[idx] % num_cols);
// }
//
// // Pipeline host che orchestra la compressione finale sfruttando CUB e i kernel sopra definiti
// void finalize_einsum_to_csr(
//     COOElement* d_coo_workspace, int nnz_generated,
//     int out_rows, int out_cols, int* d_out_dims,
//     int*& d_csr_row_ptr, int*& d_csr_col_ind, float*& d_csr_vals, int& final_nnz)
// {
//     if (nnz_generated == 0) {
//         final_nnz = 0;
//         cudaMalloc(&d_csr_row_ptr, (out_rows + 1) * sizeof(int));
//         cudaMemset(d_csr_row_ptr, 0, (out_rows + 1) * sizeof(int));
//         return;
//     }
//
//     uint64_t* d_keys_raw;
//     float* d_vals_raw;
//     cudaMalloc(&d_keys_raw, nnz_generated * sizeof(uint64_t));
//     cudaMalloc(&d_vals_raw, nnz_generated * sizeof(float));
//
//     int block = 256;
//     int grid = (nnz_generated + block - 1) / block;
//
//     linearize_coo_keys_kernel<<<grid, block>>>(d_coo_workspace, d_keys_raw, d_vals_raw, nnz_generated, 2, d_out_dims);
//
//     thrust::device_ptr<uint64_t> dev_keys(d_keys_raw);
//     thrust::device_ptr<float> dev_vals(d_vals_raw);
//     thrust::sort_by_key(thrust::device, dev_keys, dev_keys + nnz_generated, dev_vals);
//
//     uint64_t* d_unique_keys;
//     float* d_unique_vals;
//     cudaMalloc(&d_unique_keys, nnz_generated * sizeof(uint64_t));
//     cudaMalloc(&d_unique_vals, nnz_generated * sizeof(float));
//
//     int* d_num_runs_out;
//     cudaMalloc(&d_num_runs_out, sizeof(int));
//
//     void* d_temp_storage = nullptr;
//     size_t temp_storage_bytes = 0;
//     cub::DeviceReduce::ReduceByKey(d_temp_storage, temp_storage_bytes, d_keys_raw, d_unique_keys, d_vals_raw, d_unique_vals, d_num_runs_out, cub::Sum(), nnz_generated);
//     cudaMalloc(&d_temp_storage, temp_storage_bytes);
//     cub::DeviceReduce::ReduceByKey(d_temp_storage, temp_storage_bytes, d_keys_raw, d_unique_keys, d_vals_raw, d_unique_vals, d_num_runs_out, cub::Sum(), nnz_generated);
//
//     cudaMemcpy(&final_nnz, d_num_runs_out, sizeof(int), cudaMemcpyDeviceToHost);
//
//     cudaMalloc(&d_csr_row_ptr, (out_rows + 1) * sizeof(int));
//     cudaMalloc(&d_csr_col_ind, final_nnz * sizeof(int));
//     d_csr_vals = d_unique_vals;
//
//     int grid_final = (final_nnz + block - 1) / block;
//     generate_csr_row_ptr_kernel<<<grid_final, block>>>(d_unique_keys, d_csr_row_ptr, final_nnz, out_rows, out_cols);
//     extract_csr_col_ind_kernel<<<grid_final, block>>>(d_unique_keys, d_csr_col_ind, final_nnz, out_cols);
//
//     cudaFree(d_keys_raw);
//     cudaFree(d_vals_raw);
//     cudaFree(d_unique_keys);
//     cudaFree(d_temp_storage);
//     cudaFree(d_num_runs_out);
// }

// #include "csf_einsum.cuh"
#include <cstdio>

#include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"

/**
 * Kernel CUDA per l'attraversamento iterativo dell'albero CSF con array statici.
 */
__global__ void einsum_universal_csf_kernel(
    const int* __restrict__ all_ptrs,
    const int* __restrict__ all_indices,
    const float* __restrict__ csf_vals,
    const float* __restrict__ dense_input,
    EinsumPlan plan, // Passato per valore (allocato nei registri/constant memory)
    UniversalCSF csf, // Passato per valore
    int root_size,
    int* __restrict__ coo_out_indices,
    float* __restrict__ coo_out_vals,
    int* __restrict__ global_output_counter)
{
    int root_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (root_idx >= root_size) return;

    // Stack statici locali al thread (Dimensione massima fissa 7)
    int fiber_pos_stack[MAX_TENSOR_ORDER];
    int fiber_end_stack[MAX_TENSOR_ORDER];
    int current_indices[MAX_TENSOR_ORDER];

    // Inizializzazione del Livello Radice (Livello 0)
    int lvl = 0;

    int root_ptr_start = csf.ptr_offsets[0] + root_idx;
    fiber_pos_stack[0] = all_ptrs[root_ptr_start];
    fiber_end_stack[0] = all_ptrs[root_ptr_start + 1];

    if (fiber_pos_stack[0] >= fiber_end_stack[0]) return;

    // Loop di backtracking ottimizzato senza ricorsione
    while (lvl >= 0) {
        if (lvl == csf.order - 1) {
            // Livello foglia: eseguiamo la contrazione lungo la fibra finale
            int start_idx = fiber_pos_stack[lvl];
            int end_idx = fiber_end_stack[lvl];

            for (int e = start_idx; e < end_idx; ++e) {
                int leaf_idx = all_indices[csf.ind_offsets[lvl] + e];
                current_indices[lvl] = leaf_idx;

                // Calcolo dello stride sull'operando denso usando registri statici
                int current_dense_idx = 0;
                for (int c = 0; c < plan.num_contracted_modes; ++c) {
                    int target_lvl = plan.contracted_levels[c];
                    current_dense_idx += current_indices[target_lvl] * plan.dense_dim_strides[c];
                }

                float val = csf_vals[e] * dense_input[current_dense_idx];

                if (val != 0.0f) {
                    int out_slot = atomicAdd(global_output_counter, 1);
                    coo_out_vals[out_slot] = val;

                    // Copia le coordinate nel formato COO di output
                    for (int d = 0; d < csf.order; ++d) {
                        coo_out_indices[out_slot * csf.order + d] = current_indices[d];
                    }
                }
            }
            // Risalita (Backtrack) al livello precedente
            lvl--;
        }
        // Sostituisci la logica del blocco 'else' (nodi interni) nel tuo csf_einsum.cu con questa:
        else {
            int curr_pos = fiber_pos_stack[lvl];
            int end_pos = fiber_end_stack[lvl];

            if (curr_pos < end_pos) {
                // Estrai l'indice reale a questo livello
                current_indices[lvl] = all_indices[csf.ind_offsets[lvl] + curr_pos];

                int next_lvl = lvl + 1;
                // Calcola dove puntare nel livello successivo
                int next_ptr_lookup = csf.ptr_offsets[next_lvl] + curr_pos;

                int next_start = all_ptrs[next_ptr_lookup];
                int next_end = all_ptrs[next_ptr_lookup + 1];

                // Avanziamo la posizione per il backtracking futuro di QUESTO livello
                fiber_pos_stack[lvl]++;

                if (next_start < next_end) {
                    // Scendiamo al livello successivo aggiornando lo stack
                    lvl = next_lvl;
                    fiber_pos_stack[lvl] = next_start;
                    fiber_end_stack[lvl] = next_end;
                } else {
                    // Sotto-albero vuoto (foglia senza figli reali o vicolo cieco)
                    // Non scendiamo, il prossimo ciclo proverà il prossimo elemento di questo livello
                }
            } else {
                // Fibra di questo livello terminata, risaliamo (Backtrack)
                lvl--;
            }
        }

        // else {
        //     // Nodo interno: processiamo l'indice corrente del livello attuale
        //     int curr_pos = fiber_pos_stack[lvl];
        //     int end_pos = fiber_end_stack[lvl];
        //
        //     if (curr_pos < end_pos) {
        //         current_indices[lvl] = all_indices[csf.ind_offsets[lvl] + curr_pos];
        //
        //         // Prepariamo la discesa al livello successivo
        //         int next_lvl = lvl + 1;
        //         int next_ptr_lookup = csf.ptr_offsets[next_lvl] + curr_pos;
        //
        //         int next_start = all_ptrs[next_ptr_lookup];
        //         int next_end = all_ptrs[next_ptr_lookup + 1];
        //
        //         if (next_start < next_end) {
        //             // Avanziamo l'indice del livello corrente per i futuri backtracking
        //             fiber_pos_stack[lvl]++;
        //
        //             // Scendiamo nell'albero
        //             lvl = next_lvl;
        //             fiber_pos_stack[lvl] = next_start;
        //             fiber_end_stack[lvl] = next_end;
        //         } else {
        //             // Sotto-albero vuoto, passa al prossimo nodo sulla stessa fibra
        //             fiber_pos_stack[lvl]++;
        //         }
        //     } else {
        //         // Fibra completata, risaliamo di un livello
        //         lvl--;
        //     }
        }
    }


// Wrapper per l'invocazione da host C++
void launch_csf_einsum(
    const UniversalCSF& csf,
    const EinsumPlan& plan,
    const float* d_dense_input,
    int root_size,
    int* d_out_indices,
    float* d_out_vals,
    int* d_global_counter)
{
    int threads = 128;
    int blocks = (root_size + threads - 1) / threads;

    einsum_universal_csf_kernel<<<blocks, threads>>>(
        csf.d_all_ptrs, csf.d_all_indices, csf.d_vals,
        d_dense_input, plan, csf, root_size,
        d_out_indices, d_out_vals, d_global_counter
    );
}
