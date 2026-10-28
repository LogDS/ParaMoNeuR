
#include "logds/paramoneur/engine/mdd/mdd_coo_kernel.cuh"

__global__ void initMaskKernel(bool* d_mask, long long total_assignments) {
    long long idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < total_assignments) {
        d_mask[idx] = true;
    }
}

__global__ void queryBatchKernel(const int* d_queries, const bool* d_mask, bool* d_results,
                                 int num_vars, int domain_size, int num_queries) {
    int q_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (q_idx < num_queries) {
        long long linear_idx = 0;
        for (int i = 0; i < num_vars; ++i) {
            int val = d_queries[q_idx * num_vars + i];
            linear_idx = linear_idx * domain_size + val;
        }
        d_results[q_idx] = d_mask[linear_idx];
    }
}

__global__ void removeVariableConstantKernel(bool* d_mask, long long total_assignments,
                                              int num_vars, int domain_size,
                                              int target_var, int constant_val) {
    long long idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < total_assignments) {
        // Calcoliamo il divisore e il modulo per estrarre il valore della variabile in posizione 'target_var'
        // Es: in base D, la variabile k-esima si isola dividendo per D^(N - 1 - k) e facendo il modulo D
        long long current = idx;
        int steps_from_right = num_vars - 1 - target_var;

        for (int i = 0; i < steps_from_right; ++i) {
            current /= domain_size;
        }
        int var_value = current % domain_size;

        // Se la variabile esaminata ha il valore costante 'constant_val', invalidiamo l'assegnamento
        if (var_value == constant_val) {
            d_mask[idx] = false;
        }
    }
}