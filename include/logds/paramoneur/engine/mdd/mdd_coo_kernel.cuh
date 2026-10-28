//
// Created by gyankos on 28/10/26.
//

#ifndef PARAMONEUR2_MDD_COO_KERNEL_CUH
#define PARAMONEUR2_MDD_COO_KERNEL_CUH

#include <cuda_runtime.h>

// Inizializza la maschera di ammissibilità ponendo tutti i cammini a true
__global__ void initMaskKernel(bool* d_mask, long long total_assignments);

// Esegue il controllo di ammissibilità in parallelo per un batch di query
__global__ void queryBatchKernel(const int* d_queries, const bool* d_mask, bool* d_results,
                                 int num_vars, int domain_size, int num_queries);

// NUOVO: Kernel per invalidare tutti i cammini dove la variabile 'target_var' è uguale a 'constant_val'
__global__ void removeVariableConstantKernel(bool* d_mask, long long total_assignments,
                                              int num_vars, int domain_size,
                                              int target_var, int constant_val);


#endif //PARAMONEUR2_MDD_COO_KERNEL_CUH
