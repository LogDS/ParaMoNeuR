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

__global__ void einsum_universal_sum_reduction_leaf_kernel(
    const float* __restrict__ csf_vals,
    const float* __restrict__ dense_input,
    const int* __restrict__ coordinate_matrix,
    SumReductionPlan plan,
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

void launch_csf_einsum_tensor_sum_reduction(
    const UniversalCSF& csf,
    const SumReductionPlan& plan,
    const float* d_dense_input,
    UnifiedCOOOutput& out)
{
    int threads = 128;
    int blocks = (csf.nnz + threads - 1) / threads;
    einsum_universal_sum_reduction_leaf_kernel<<<blocks, threads>>>(
        csf.d_vals, d_dense_input, csf.d_coordinate_matrix,
        plan, csf, out.d_out_indices, out.d_out_vals, out.d_global_counter, out.out_order
    );
}

#include "csf_einsum.cuh"


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


/**
 * Kernel CUDA per la contrazione Einsum Pura su due tensori sparsi.
 * Parallelizzazione basata sul cross-matching degli elementi foglia di A e B.
 * Se le coordinate degli assi contratti coincidono, esegue il prodotto e proietta le coordinate superstiti.
 */
#ifdef SLOW
__global__ void einsum_two_tensors_leaf_kernel(
    UniversalCSF csf_A,
    UniversalCSF csf_B,
    EinsumPlan plan,
    int* __restrict__ coo_out_indices,
    float* __restrict__ coo_out_vals,
    int* __restrict__ global_output_counter,
    int max_output_nnz)
{
    // Parallelizziamo mappando una griglia bidimensionale virtuale o linearizzando il prodotto delle foglie
    int idx_A = blockIdx.x * blockDim.x + threadIdx.x;
    int idx_B = blockIdx.y * blockDim.y + threadIdx.y;

    if (idx_A >= csf_A.nnz || idx_B >= csf_B.nnz) return;

    // 1. Estrazione delle coordinate complete per l'elemento corrente di A e di B
    int coords_A[MAX_TENSOR_ORDER];
    int coords_B[MAX_TENSOR_ORDER];

    for (int d = 0; d < csf_A.order; ++d) {
        coords_A[d] = csf_A.d_coordinate_matrix[idx_A * csf_A.order + d];
    }
    for (int d = 0; d < csf_B.order; ++d) {
        coords_B[d] = csf_B.d_coordinate_matrix[idx_B * csf_B.order + d];
    }

    // 2. Controllo di corrispondenza geometrica degli assi contratti (Intersection Match)
    bool is_match = true;

    // Per ogni modalità contratta definita dal piano, controlliamo se A e B condividono lo stesso indice
    for (int c = 0; c < plan.num_contracted_modes; ++c) {
        int global_mode = plan.global_contracted_levels[c];

        // Trova quale asse di A e B corrisponde a questo asse contratto globale
        int local_axis_A = -1;
        int local_axis_B = -1;
        for (int d = 0; d < csf_A.order; ++d) {
            if (plan.map_A_to_global[d] == global_mode) { local_axis_A = d; break; }
        }
        for (int d = 0; d < csf_B.order; ++d) {
            if (plan.map_B_to_global[d] == global_mode) { local_axis_B = d; break; }
        }

        // Se l'asse è presente in entrambi ma le coordinate differiscono, non c'è intersezione
        if (local_axis_A != -1 && local_axis_B != -1) {
            if (coords_A[local_axis_A] != coords_B[local_axis_B]) {
                is_match = false;
                break;
            }
        }
    }

    // Controlliamo anche che le modalità comuni NON contratte (che vanno in output) combacino
    if (is_match) {
        for (int d_A = 0; d_A < csf_A.order; ++d_A) {
            int global_mode_A = plan.map_A_to_global[d_A];
            for (int d_B = 0; d_B < csf_B.order; ++d_B) {
                int global_mode_B = plan.map_B_to_global[d_B];
                if (global_mode_A == global_mode_B) {
                    if (coords_A[d_A] != coords_B[d_B]) {
                        is_match = false;
                        break;
                    }
                }
            }
            if (!is_match) break;
        }
    }

    // 3. Se la struttura geometrica interseca, calcoliamo il prodotto strutturale
    if (is_match) {
        float prodotto = csf_A.d_vals[idx_A] * csf_B.d_vals[idx_B];

        if (prodotto != 0.0f) {
            int out_slot = atomicAdd(global_output_counter, 1);
            if (out_slot >= max_output_nnz) return; // Protezione out-of-bounds

            coo_out_vals[out_slot] = prodotto;

            // Costruiamo le coordinate di output proiettando le label salvate nel piano
            for (int d_out = 0; d_out < plan.order_Out; ++d_out) {
                int target_global_mode = plan.map_out_to_global[d_out];
                int final_coord = 0;

                // Cerca se l'indice appartiene ad A o a B per estrarre la coordinata reale
                bool found = false;
                for (int d_A = 0; d_A < csf_A.order; ++d_A) {
                    if (plan.map_A_to_global[d_A] == target_global_mode) {
                        final_coord = coords_A[d_A];
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    for (int d_B = 0; d_B < csf_B.order; ++d_B) {
                        if (plan.map_B_to_global[d_B] == target_global_mode) {
                            final_coord = coords_B[d_B];
                            break;
                        }
                    }
                }
                coo_out_indices[out_slot * plan.order_Out + d_out] = final_coord;
            }
        }
    }
}

void launch_csf_einsum_two_tensors(
    const UniversalCSF& csf_A,
    const UniversalCSF& csf_B,
    const EinsumPlan& plan,
    UnifiedCOOOutput& out)
{
    // Configurazione bidimensionale dei blocchi CUDA per mappare la combinazione delle foglie [NNZ_A x NNZ_B]
    dim3 threadsPerBlock(16, 16);
    dim3 numBlocks(
        (csf_A.nnz + threadsPerBlock.x - 1) / threadsPerBlock.x,
        (csf_B.nnz + threadsPerBlock.y - 1) / threadsPerBlock.y
    );

    einsum_two_tensors_leaf_kernel<<<numBlocks, threadsPerBlock>>>(
        csf_A, csf_B, plan, out.d_out_indices, out.d_out_vals, out.d_global_counter, out.max_nnz
    );
}

#endif


// /**
//  * Compara due tuple di coordinate a MAX_TENSOR_ORDER dimensioni.
//  * Ritorna -1 se A < B, 1 se A > B, 0 se A == B (lessicografico).
//  */
// __device__ inline int compare_coordinates(const int* coord_A, const int* coord_B, int order) {
//     for (int d = 0; d < order; ++d) {
//         if (coord_A[d] < coord_B[d]) return -1;
//         if (coord_A[d] > coord_B[d]) return 1;
//     }
//     return 0;
// }
//
// /**
//  * Kernel CUDA ad Alte Prestazioni.
//  * Parallelizzazione lineare sulle foglie di A con Ricerca Binaria su B.
//  * Complessità abbattuta a O(NNZ_A * log(NNZ_B)). Zero Warp Divergence.
//  */
// __global__ void einsum_two_tensors_binary_search_kernel(
//     UniversalCSF csf_A,
//     UniversalCSF csf_B,
//     EinsumPlan plan,
//     int* __restrict__ coo_out_indices,
//     float* __restrict__ coo_out_vals,
//     int* __restrict__ global_output_counter,
//     int max_output_nnz)
// {
//     // 1 Thread per ogni elemento non-zero di A
//     int idx_A = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx_A >= csf_A.nnz) return;
//
//     // Caricamento della tupla di coordinate di A nei registri locali
//     int coords_A[MAX_TENSOR_ORDER];
//     for (int d = 0; d < csf_A.order; ++d) {
//         coords_A[d] = csf_A.d_coordinate_matrix[idx_A * csf_A.order + d];
//     }
//
//     // =========================================================================
//     // RICERCA BINARIA MULTI-DIMENSIONALE SU B
//     // =========================================================================
//     int low = 0;
//     int high = csf_B.nnz - 1;
//     int idx_B = -1;
//
//     while (low <= high) {
//         int mid = low + (high - low) / 2;
//
//         // Estraiamo la tupla di B per il confronto
//         int coords_B[MAX_TENSOR_ORDER];
//         for (int d = 0; d < csf_B.order; ++d) {
//             coords_B[d] = csf_B.d_coordinate_matrix[mid * csf_B.order + d];
//         }
//
//         // Confronto lessicografico completo
//         int cmp = compare_coordinates(coords_A, coords_B, csf_A.order);
//
//         if (cmp == 0) {
//             idx_B = mid; // Trovato il match perfetto strutturale!
//             break;
//         } else if (cmp < 0) {
//             high = mid - 1;
//         } else {
//             low = mid + 1;
//         }
//     }
//
//     // Se non esiste corrispondenza geometrica, usciamo immediatamente
//     if (idx_B == -1) return;
//
//     // =========================================================================
//     // CALCOLO ED EMISSIONE ATOMICA (Eseguito solo per i match reali)
//     // =========================================================================
//     float prodotto = csf_A.d_vals[idx_A] * csf_B.d_vals[idx_B];
//     if (prodotto != 0.0f) {
//         int out_slot = atomicAdd(global_output_counter, 1);
//         if (out_slot >= max_output_nnz) return;
//
//         coo_out_vals[out_slot] = prodotto;
//
//         // Scrittura delle sole coordinate superstiti richieste dall'output del piano ("abe")
//         for (int d_out = 0; d_out < plan.order_Out; ++d_out) {
//             int target_global_mode = plan.map_out_to_global[d_out];
//             // Poiché A e B coincidono perfettamente, proiettiamo direttamente da coords_A
//             coo_out_indices[out_slot * plan.order_Out + d_out] = coords_A[target_global_mode];
//         }
//     }
// }
//
// void launch_csf_einsum_two_tensors(
//     const UniversalCSF& csf_A,
//     const UniversalCSF& csf_B,
//     const EinsumPlan& plan,
//     UnifiedCOOOutput& out)
// {
//     int threadsPerBlock = 256;
//     int numBlocks = (csf_A.nnz + threadsPerBlock - 1) / threadsPerBlock;
//
//     einsum_two_tensors_binary_search_kernel<<<numBlocks, threadsPerBlock>>>(
//         csf_A, csf_B, plan, out.d_out_indices, out.d_out_vals, out.d_global_counter, out.max_nnz
//     );
// }
/**
 * Compara due tuple di coordinate a MAX_TENSOR_ORDER dimensioni.
 * Ritorna -1 se A < B, 1 se A > B, 0 se A == B (lessicografico).
 */
__device__ inline int compare_coordinates(const int* coord_A, const int* coord_B, int order) {
    for (int d = 0; d < order; ++d) {
        if (coord_A[d] < coord_B[d]) return -1;
        if (coord_A[d] > coord_B[d]) return 1;
    }
    return 0;
}

/**
 * Kernel CUDA Universale ad Alte Prestazioni.
 * Parallelizzazione 1D sulle foglie di A con ricerca binaria a permutazione dinamica su B.
 */
__global__ void einsum_two_tensors_binary_search_kernel(
    UniversalCSF csf_A,
    UniversalCSF csf_B,
    EinsumPlan plan,
    int* __restrict__ coo_out_indices,
    float* __restrict__ coo_out_vals,
    int* __restrict__ global_output_counter,
    int max_output_nnz)
{
    // 1 Thread per ogni elemento non-zero di A
    int idx_A = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx_A >= csf_A.nnz) return;

    // 1. Caricamento delle coordinate fisiche locali di A
    int coords_A[MAX_TENSOR_ORDER];
    for (int d = 0; d < csf_A.order; ++d) {
        coords_A[d] = csf_A.d_coordinate_matrix[idx_A * csf_A.order + d];
    }

    // 2. COSTRUZIONE DINAMICA DEL TARGET DI RICERCA NEL LAYOUT DI B
    // Ricostruiamo nei registri come si presenterebbe l'elemento di A se fosse dentro B,
    // rispettando al 100% le label e l'ordinamento meno o più denso di B.
    int target_B_coords[MAX_TENSOR_ORDER];

    for (int d_B = 0; d_B < csf_B.order; ++d_B) {
        int global_mode_B = plan.map_B_to_global[d_B];

        // Cerchiamo quale asse locale di A corrisponde a questa modalità globale di B
        int local_axis_A = -1;
        for (int d_A = 0; d_A < csf_A.order; ++d_A) {
            if (plan.map_A_to_global[d_A] == global_mode_B) {
                local_axis_A = d_A;
                break;
            }
        }

        // Assegniamo il valore coordinato corrispondente
        if (local_axis_A != -1) {
            target_B_coords[d_B] = coords_A[local_axis_A];
        } else {
            target_B_coords[d_B] = 0; // Gestione difensiva assi non condivisi
        }
    }

    // =========================================================================
    // RICERCA BINARIA PERMUTATA SU B
    // =========================================================================
    int low = 0;
    int high = csf_B.nnz - 1;
    int idx_B = -1;

    while (low <= high) {
        int mid = low + (high - low) / 2;

        // Estraiamo le coordinate fisiche correnti di B alla posizione mediana
        int coords_B[MAX_TENSOR_ORDER];
        for (int d = 0; d < csf_B.order; ++d) {
            coords_B[d] = csf_B.d_coordinate_matrix[mid * csf_B.order + d];
        }

        // Confronto lessicografico perfetto: target_B_coords e coords_B condividono lo stesso identico ordine!
        int cmp = compare_coordinates(target_B_coords, coords_B, csf_B.order);

        if (cmp == 0) {
            idx_B = mid; // Match geometrico perfetto trovato!
            break;
        } else if (cmp < 0) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    // Se l'elemento di A non ha un corrispettivo geometrico in B, abortiamo il thread
    if (idx_B == -1) return;

    // =========================================================================
    // CALCOLO PRODOTTO ED EMISSIONE ATOMICA COO
    // =========================================================================
    float prodotto = csf_A.d_vals[idx_A] * csf_B.d_vals[idx_B];
    if (prodotto != 0.0f) {
        int out_slot = atomicAdd(global_output_counter, 1);
        if (out_slot >= max_output_nnz) return;

        coo_out_vals[out_slot] = prodotto;

        // Proiezione finale delle sole coordinate superstiti esterne richieste dall'output ("abe")
        for (int d_out = 0; d_out < plan.order_Out; ++d_out) {
            int target_global_mode = plan.map_out_to_global[d_out];
            int final_coord = 0;

            // Leggiamo la coordinata reale dalla mappa globale validata
            for (int d_A = 0; d_A < csf_A.order; ++d_A) {
                if (plan.map_A_to_global[d_A] == target_global_mode) {
                    final_coord = coords_A[d_A];
                    break;
                }
            }
            coo_out_indices[out_slot * plan.order_Out + d_out] = final_coord;
        }
    }
}

void launch_csf_einsum_two_tensors(
    const UniversalCSF& csf_A,
    const UniversalCSF& csf_B,
    const EinsumPlan& plan,
    UnifiedCOOOutput& out)
{
    int threadsPerBlock = 256;
    int numBlocks = (csf_A.nnz + threadsPerBlock - 1) / threadsPerBlock;

    einsum_two_tensors_binary_search_kernel<<<numBlocks, threadsPerBlock>>>(
        csf_A, csf_B, plan, out.d_out_indices, out.d_out_vals, out.d_global_counter, out.max_nnz
    );
}
