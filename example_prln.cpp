//
// Created by gyankos on 27/10/26.
//

#include <iostream>
#include <vector>
#include <cuda_runtime.h>
#include "logds/paramoneur/engine/prln_step.cuh"

#define CUDA_CHECK(call) \
    if((call) != cudaSuccess) { \
        std::cerr << "CUDA Error at line " << __LINE__ << ": " << cudaGetErrorString(cudaGetLastError()) << std::endl; \
        exit(1); \
    }

int main() {
    // Dimensioni (num_cols deve essere pari per permettere dim(C) = dim(X)/2)
    const int num_rows = 4;
    const int num_cols = 4;
    const int dim_C = num_cols / 2;

    const float alpha = 0.5f;
    const float theta = 0.2f;

    // --- Dati Host (CPU) ---
    std::vector<int>   h_S_rowPtr = {0, 1, 2, 3, 4};
    std::vector<int>   h_S_colInd = {0, 1, 2, 3};
    std::vector<float> h_S_val    = {1.0f, 1.0f, 1.0f, 1.0f};

    std::vector<int>   h_T_rowPtr = {0, 1, 2, 3, 4};
    std::vector<int>   h_T_colInd = {0, 1, 2, 3};
    std::vector<float> h_T_val    = {1.0f, 1.0f, 1.0f, 1.0f};

    // Configurato h_X in modo che:
    // d=0 (0.5 > 0.2) e d=2 (0.6 > 0.2) -> COPPIA 1 VALIDA (C[0] = 1)
    // d=1 (0.1 < 0.2) e d=3 (0.7 > 0.2) -> COPPIA 2 INVALIDA (C[1] = 0)
    // Risultato atteso: incdec = 1
    std::vector<float> h_X = {0.5f, 0.1f, 0.6f, 0.7f};
    std::vector<float> h_Y = {0.8f, 0.8f, 0.8f, 0.8f};
    std::vector<float> h_H = {0.2f, 0.2f, 0.2f, 0.2f};
    std::vector<float> h_K = {0.1f, 0.1f, 0.1f, 0.1f};
    std::vector<float> h_W = {2.0f, 2.0f, 2.0f, 2.0f};
    std::vector<int>   h_V = {0, 0, 1, 2};

    // Vettori Output (Host)
    std::vector<float> h_Z(num_rows, 0.0f);
    std::vector<float> h_X_prime(num_cols, 0.0f);
    std::vector<float> h_Y_prime(num_cols, 0.0f);
    std::vector<float> h_C(dim_C, 0.0f);
    int h_incdec = 0;

    // --- Allocazione Memoria Device (GPU) ---
    int *d_S_rowPtr, *d_S_colInd, *d_T_rowPtr, *d_T_colInd, *d_V, *d_incdec;
    float *d_S_val, *d_T_val, *d_X, *d_Y, *d_H, *d_K, *d_W, *d_Z, *d_X_prime, *d_Y_prime, *d_C;

    CUDA_CHECK(cudaMalloc(&d_S_rowPtr, h_S_rowPtr.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_S_colInd, h_S_colInd.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_S_val, h_S_val.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_T_rowPtr, h_T_rowPtr.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_T_colInd, h_T_colInd.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_T_val, h_T_val.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_X, h_X.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_Y, h_Y.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_H, h_H.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_K, h_K.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_W, h_W.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_V, h_V.size() * sizeof(int)));

    // Allocazione nuovi output
    CUDA_CHECK(cudaMalloc(&d_Z, h_Z.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_X_prime, h_X_prime.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_Y_prime, h_Y_prime.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_C, h_C.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_incdec, sizeof(int)));

    // --- Copia dati Host -> Device ---
    CUDA_CHECK(cudaMemcpy(d_S_rowPtr, h_S_rowPtr.data(), h_S_rowPtr.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_S_colInd, h_S_colInd.data(), h_S_colInd.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_S_val, h_S_val.data(), h_S_val.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_T_rowPtr, h_T_rowPtr.data(), h_T_rowPtr.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_T_colInd, h_T_colInd.data(), h_T_colInd.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_T_val, h_T_val.data(), h_T_val.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_X, h_X.data(), h_X.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_Y, h_Y.data(), h_Y.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_H, h_H.data(), h_H.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_K, h_K.data(), h_K.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_W, h_W.data(), h_W.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_V, h_V.data(), h_V.size() * sizeof(int), cudaMemcpyHostToDevice));

    // --- Invocazione Pipeline ---
    std::cout << "Esecuzione della pipeline estesa da codice C++..." << std::endl;
    launch_prln_step_wrapper(
        d_S_rowPtr, d_S_colInd, d_S_val,
        d_T_rowPtr, d_T_colInd, d_T_val,
        d_X, d_Y, d_H, d_K, d_W, d_V,
        d_Z, d_X_prime, d_Y_prime,
        d_C, d_incdec,
        alpha, theta, num_rows, num_cols
    );

    // --- Recupero dei Risultati ---
    CUDA_CHECK(cudaMemcpy(h_Z.data(), d_Z, h_Z.size() * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_X_prime.data(), d_X_prime, h_X_prime.size() * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_Y_prime.data(), d_Y_prime, h_Y_prime.size() * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_C.data(), d_C, h_C.size() * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&h_incdec, d_incdec, sizeof(int), cudaMemcpyDeviceToHost));

    // --- Stampa dei Risultati ---
    std::cout << "\n--- RISULTATI COMPLETI ---" << std::endl;
    for(int i = 0; i < num_rows; ++i) std::cout << "Z[" << i << "] = " << h_Z[i] << std::endl;
    std::cout << std::endl;
    for(int i = 0; i < num_cols; ++i) std::cout << "X'[" << i << "] = " << h_X_prime[i] << std::endl;
    std::cout << std::endl;
    for(int i = 0; i < num_cols; ++i) std::cout << "Y'[" << i << "] = " << h_Y_prime[i] << std::endl;
    std::cout << std::endl;
    for(int i = 0; i < dim_C; ++i)   std::cout << "C[" << i << "] = " << h_C[i] << std::endl;

    std::cout << "\n>>> incdec = " << h_incdec << " <<<" << std::endl;

    // --- Pulizia Memoria Device ---
    cudaFree(d_S_rowPtr); cudaFree(d_S_colInd); cudaFree(d_S_val);
    cudaFree(d_T_rowPtr); cudaFree(d_T_colInd); cudaFree(d_T_val);
    cudaFree(d_X); cudaFree(d_Y); cudaFree(d_H); cudaFree(d_K); cudaFree(d_W); cudaFree(d_V);
    cudaFree(d_Z); cudaFree(d_X_prime); cudaFree(d_Y_prime); cudaFree(d_C); cudaFree(d_incdec);

    return 0;
}
