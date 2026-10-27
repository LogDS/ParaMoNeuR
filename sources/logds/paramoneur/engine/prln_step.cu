#include "../../../../include/logds/paramoneur/engine/prln_step.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

// KERNEL 1: Calcolo Fuso di Z[r]
__global__ void compute_Z_kernel(
    const int* S_rowPtr, const int* S_colInd, const float* S_val, const float* X,
    const int* T_rowPtr, const int* T_colInd, const float* T_val, const float* Y,
    const float* H, const float* K, const float* W,
    float* Z, float alpha, int num_rows)
{
    int r = blockIdx.x * blockDim.x + threadIdx.x;
    if (r < num_rows) {
        float sum_S = 0.0f;
        int S_start = S_rowPtr[r];
        int S_end   = S_rowPtr[r + 1];
        for (int i = S_start; i < S_end; ++i) {
            sum_S += S_val[i] * X[S_colInd[i]];
        }
        float bias_S = 1.0f - alpha * H[r];
        float rho_S = fminf(1.0f, fmaxf(0.0f, sum_S + bias_S));

        float sum_T = 0.0f;
        int T_start = T_rowPtr[r];
        int T_end   = T_rowPtr[r + 1];
        for (int i = T_start; i < T_end; ++i) {
            sum_T += T_val[i] * Y[T_colInd[i]];
        }
        float bias_T = 1.0f - K[r];
        float rho_T = fminf(1.0f, fmaxf(0.0f, sum_T + bias_T));

        Z[r] = rho_S * rho_T * W[r];
    }
}

// KERNEL 2: Inizializzatore per X_prime
__global__ void init_X_prime_kernel(float* X_prime, float theta, int num_cols) {
    int d = blockIdx.x * blockDim.x + threadIdx.x;
    if (d < num_cols) {
        X_prime[d] = theta;
    }
}

// KERNEL 3: Scatter-Max Condizionale per X'[d]
__global__ void compute_X_prime_kernel(
    const float* Z, const int* V, float* X_prime, float theta, int num_rows)
{
    int r = blockIdx.x * blockDim.x + threadIdx.x;
    if (r < num_rows) {
        int d = V[r];
        float val = Z[r];
        if (val >= theta) {
            atomicMax((int*)&X_prime[d], __float_as_int(val));
        }
    }
}

// KERNEL 4: Aggiornamento Elementwise di Y'[d]
__global__ void compute_Y_prime_kernel(
    const float* X_prime, const float* Y, float* Y_prime, float theta, int num_cols)
{
    int d = blockIdx.x * blockDim.x + threadIdx.x;
    if (d < num_cols) {
        float indicator = (X_prime[d] <= theta) ? 1.0f : 0.0f;
        Y_prime[d] = fminf(indicator, Y[d]);
    }
}

// KERNEL 5 NUOVO: Calcolo di C[d] e riduzione atomica per incdec
__global__ void compute_C_and_incdec_kernel(
    const float* X, float* C, int* incdec, float theta, int dim_C)
{
    int d = blockIdx.x * blockDim.x + threadIdx.x;
    if (d < dim_C) {
        // s = dim_C (lo shift corrisponde alla metà della dimensione di X)
        int s = dim_C;

        // Verifica la condizione logica: X[d] > theta AND X[d+s] > theta
        if (X[d] > theta && X[d + s] > theta) {
            C[d] = 1.0f;
            // Incrementa in modo atomico e sicuro il contatore globale in VRAM
            atomicAdd(incdec, 1);
        } else {
            C[d] = 0.0f;
        }
    }
}

// Implementazione del Wrapper esposto al C++
void launch_prln_step_wrapper(
    int* d_S_rowPtr, int* d_S_colInd, float* d_S_val,
    int* d_T_rowPtr, int* d_T_colInd, float* d_T_val,
    float* d_X, float* d_Y, float* d_H, float* d_K, float* d_W, int* d_V,
    float* d_Z, float* d_X_prime, float* d_Y_prime,
    float* d_C, int* d_incdec,
    float alpha, float theta, int num_rows, int num_cols)
{
    int blockSize = 256;
    int gridSizeRows = (num_rows + blockSize - 1) / blockSize;
    int gridSizeCols = (num_cols + blockSize - 1) / blockSize;

    int dim_C = num_cols / 2;
    int gridSizeC = (dim_C + blockSize - 1) / blockSize;

    // Inizializzazioni preventive su GPU
    init_X_prime_kernel<<<gridSizeCols, blockSize>>>(d_X_prime, theta, num_cols);
    cudaMemset(d_incdec, 0, sizeof(int)); // Resetta il contatore incdec a 0

    // FASE 1: Calcolo Z[r]
    compute_Z_kernel<<<gridSizeRows, blockSize>>>(
        d_S_rowPtr, d_S_colInd, d_S_val, d_X,
        d_T_rowPtr, d_T_colInd, d_T_val, d_Y,
        d_H, d_K, d_W, d_Z, alpha, num_rows
    );

    // FASE 2: Scatter-Max condizionale per X'[d]
    compute_X_prime_kernel<<<gridSizeRows, blockSize>>>(
        d_Z, d_V, d_X_prime, theta, num_rows
    );

    // FASE 3: Generazione finale di Y'[d]
    compute_Y_prime_kernel<<<gridSizeCols, blockSize>>>(
        d_X_prime, d_Y, d_Y_prime, theta, num_cols
    );

    // FASE 4: Generazione di C[d] e calcolo simultaneo di incdec
    compute_C_and_incdec_kernel<<<gridSizeC, blockSize>>>(
        d_X, d_C, d_incdec, theta, dim_C
    );

    cudaDeviceSynchronize();
}
