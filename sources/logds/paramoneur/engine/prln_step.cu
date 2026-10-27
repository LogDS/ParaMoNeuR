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

__global__ void compute_C_and_accumulate_kernel(const float* X, float* C, float* incdec, float theta, int dim_C) {
    int d = blockIdx.x * blockDim.x + threadIdx.x;
    if (d < dim_C) {
        if (X[d] > theta && X[d + dim_C] > theta) {
            C[d] = 1.0f;
            atomicAdd(incdec, 1.0f); // Accumulo atomico float
        } else {
            C[d] = 0.0f;
        }
    }
}

__global__ void compute_I_and_accumulate_kernel(
    const int* M_rowPtr, const int* M_colInd, const float* M_val,
    const float* X_prime, const float* N, float* I, float* incdec, int num_rows_M)
{
    int m = blockIdx.x * blockDim.x + threadIdx.x;
    if (m < num_rows_M) {
        float sum_M = 0.0f;
        int start = M_rowPtr[m]; int end = M_rowPtr[m + 1];
        for (int i = start; i < end; ++i) {
            sum_M += M_val[i] * X_prime[M_colInd[i]];
        }
        float val_I = fminf(1.0f, fmaxf(0.0f, sum_M + 1.0f - N[m]));
        I[m] = val_I;
        atomicAdd(incdec, val_I); // Accumulo del valore continuo di I[m]
    }
}

// Implementazione del Wrapper esposto al C++
void launch_prln_step_wrapper(
int* d_S_rowPtr, int* d_S_colInd, float* d_S_val,
    int* d_T_rowPtr, int* d_T_colInd, float* d_T_val,
    int* d_M_rowPtr, int* d_M_colInd, float* d_M_val,
    float* d_X, float* d_Y, float* d_H, float* d_K, float* d_W, int* d_V, float* d_N,
    float* d_Z, float* d_X_prime, float* d_Y_prime, float* d_C, float* d_I,
    float* d_incdec,
    float alpha, float theta, int num_rows, int num_cols, int num_rows_M)
{
    int blockSize = 256;
    int gridSizeRows = (num_rows + blockSize - 1) / blockSize;
    int gridSizeCols = (num_cols + blockSize - 1) / blockSize;
    int dim_C = num_cols / 2;
    int gridSizeC = (dim_C + blockSize - 1) / blockSize;
    int gridSizeM = (num_rows_M + blockSize - 1) / blockSize;

    init_X_prime_kernel<<<gridSizeCols, blockSize>>>(d_X_prime, theta, num_cols);
    cudaMemset(d_incdec, 0, sizeof(float));

    compute_Z_kernel<<<gridSizeRows, blockSize>>>(d_S_rowPtr, d_S_colInd, d_S_val, d_X, d_T_rowPtr, d_T_colInd, d_T_val, d_Y, d_H, d_K, d_W, d_Z, alpha, num_rows);
    compute_X_prime_kernel<<<gridSizeRows, blockSize>>>(d_Z, d_V, d_X_prime, theta, num_rows);
    compute_Y_prime_kernel<<<gridSizeCols, blockSize>>>(d_X_prime, d_Y, d_Y_prime, theta, num_cols);
    compute_C_and_accumulate_kernel<<<gridSizeC, blockSize>>>(d_X, d_C, d_incdec, theta, dim_C);
    compute_I_and_accumulate_kernel<<<gridSizeM, blockSize>>>(d_M_rowPtr, d_M_colInd, d_M_val, d_X_prime, d_N, d_I, d_incdec, num_rows_M);

    cudaDeviceSynchronize();
}


// ============================================================================
// KERNELS E CODICE PER LA FUNZIONE 2 (SOLVER ITERATIVO)
// ============================================================================

__global__ void init_iterative_kernel(const float* Y, float* A, float* G, float theta, int size) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < size) {
        A[i] = (Y[i] > theta) ? 1.0f : 0.0f;
        G[i] = 0.0f; // Inizializza G a vettore nullo
    }
}

__global__ void iterative_step_kernel(
    const int* R_rowPtr, const int* R_colInd, const float* R_val,
    const float* A, float* A_new, float* G, float* d_sum_A, float theta, int size)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < size) {
        float sum_R = 0.0f;
        int start = R_rowPtr[i]; int end = R_rowPtr[i + 1];
        for (int j = start; j < end; ++j) {
            sum_R += R_val[j] * A[R_colInd[j]];
        }

        // A <= (R * A > theta)
        float a_val = (sum_R > theta) ? 1.0f : 0.0f;
        A_new[i] = a_val;

        // G <= max(G, A)
        G[i] = fmaxf(G[i], a_val);

        // Accumulo per la condizione di break locale
        atomicAdd(d_sum_A, a_val);
    }
}

void launch_iterative_inconsistency_propagation(
    int* d_R_rowPtr, int* d_R_colInd, float* d_R_val,
    float* d_Y, float* d_G, float theta, int maxiterations, int size)
{
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;

    // Buffer di lavoro interni allocati sulla GPU
    float *d_A, *d_A_new, *d_sum_A;
    cudaMalloc(&d_A, size * sizeof(float));
    cudaMalloc(&d_A_new, size * sizeof(float));
    cudaMalloc(&d_sum_A, sizeof(float));

    init_iterative_kernel<<<gridSize, blockSize>>>(d_Y, d_A, d_G, theta, size);

    for (int iter = 0; iter < maxiterations; ++iter) {
        cudaMemset(d_sum_A, 0, sizeof(float));

        // Esegue lo step di moltiplicazione e aggiornamento
        iterative_step_kernel<<<gridSize, blockSize>>>(d_R_rowPtr, d_R_colInd, d_R_val, d_A, d_A_new, d_G, d_sum_A, theta, size);
        cudaDeviceSynchronize();

        // Controlla la condizione di break copiando la somma parziale sull'Host
        float h_sum_A = 0.0f;
        cudaMemcpy(&h_sum_A, d_sum_A, sizeof(float), cudaMemcpyDeviceToHost);

        if (h_sum_A < theta) {
            break;
        }

        // Swappa i puntatori di A e A_new per la prossima iterazione
        float* temp = d_A;
        d_A = d_A_new;
        d_A_new = temp;
    }

    cudaFree(d_A); cudaFree(d_A_new); cudaFree(d_sum_A);
}