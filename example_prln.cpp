//
// Created by gyankos on 27/10/26.
//

#include <iostream>
#include <vector>
#include <cuda_runtime.h>
#include "logds/paramoneur/engine/prln_step.cuh"
#include "logds/paramoneur/engine/mdd/mdd_coo.h"
#include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"
#include "logds/paramoneur/types/SparseTensor.h"

#define CUDA_CHECK(call) \
    if((call) != cudaSuccess) { \
        std::cerr << "CUDA Error at line " << __LINE__ << ": " << cudaGetErrorString(cudaGetLastError()) << std::endl; \
        exit(1); \
    }

int main() {
    // // Dimensioni (num_cols deve essere pari per permettere dim(C) = dim(X)/2)
    // const int num_rows = 4;
    // const int num_cols = 4;
    // const int dim_C = num_cols / 2;
    //
    // const float alpha = 0.5f;
    // const float theta = 0.2f;
    //
    // // --- Dati Host (CPU) ---
    // std::vector<int>   h_S_rowPtr = {0, 1, 2, 3, 4};
    // std::vector<int>   h_S_colInd = {0, 1, 2, 3};
    // std::vector<float> h_S_val    = {1.0f, 1.0f, 1.0f, 1.0f};
    //
    // std::vector<int>   h_T_rowPtr = {0, 1, 2, 3, 4};
    // std::vector<int>   h_T_colInd = {0, 1, 2, 3};
    // std::vector<float> h_T_val    = {1.0f, 1.0f, 1.0f, 1.0f};
    //
    // // Configurato h_X in modo che:
    // // d=0 (0.5 > 0.2) e d=2 (0.6 > 0.2) -> COPPIA 1 VALIDA (C[0] = 1)
    // // d=1 (0.1 < 0.2) e d=3 (0.7 > 0.2) -> COPPIA 2 INVALIDA (C[1] = 0)
    // // Risultato atteso: incdec = 1
    // std::vector<float> h_X = {0.5f, 0.1f, 0.6f, 0.7f};
    // std::vector<float> h_Y = {0.8f, 0.8f, 0.8f, 0.8f};
    // std::vector<float> h_H = {0.2f, 0.2f, 0.2f, 0.2f};
    // std::vector<float> h_K = {0.1f, 0.1f, 0.1f, 0.1f};
    // std::vector<float> h_W = {2.0f, 2.0f, 2.0f, 2.0f};
    // std::vector<int>   h_V = {0, 0, 1, 2};
    //
    // // Vettori Output (Host)
    // std::vector<float> h_Z(num_rows, 0.0f);
    // std::vector<float> h_X_prime(num_cols, 0.0f);
    // std::vector<float> h_Y_prime(num_cols, 0.0f);
    // std::vector<float> h_C(dim_C, 0.0f);
    // int h_incdec = 0;
    //
    // // --- Allocazione Memoria Device (GPU) ---
    // int *d_S_rowPtr, *d_S_colInd, *d_T_rowPtr, *d_T_colInd, *d_V, *d_incdec;
    // float *d_S_val, *d_T_val, *d_X, *d_Y, *d_H, *d_K, *d_W, *d_Z, *d_X_prime, *d_Y_prime, *d_C;
    //
    // CUDA_CHECK(cudaMalloc(&d_S_rowPtr, h_S_rowPtr.size() * sizeof(int)));
    // CUDA_CHECK(cudaMalloc(&d_S_colInd, h_S_colInd.size() * sizeof(int)));
    // CUDA_CHECK(cudaMalloc(&d_S_val, h_S_val.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_T_rowPtr, h_T_rowPtr.size() * sizeof(int)));
    // CUDA_CHECK(cudaMalloc(&d_T_colInd, h_T_colInd.size() * sizeof(int)));
    // CUDA_CHECK(cudaMalloc(&d_T_val, h_T_val.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_X, h_X.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_Y, h_Y.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_H, h_H.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_K, h_K.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_W, h_W.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_V, h_V.size() * sizeof(int)));
    //
    // // Allocazione nuovi output
    // CUDA_CHECK(cudaMalloc(&d_Z, h_Z.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_X_prime, h_X_prime.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_Y_prime, h_Y_prime.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_C, h_C.size() * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_incdec, sizeof(int)));
    //
    // // --- Copia dati Host -> Device ---
    // CUDA_CHECK(cudaMemcpy(d_S_rowPtr, h_S_rowPtr.data(), h_S_rowPtr.size() * sizeof(int), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_S_colInd, h_S_colInd.data(), h_S_colInd.size() * sizeof(int), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_S_val, h_S_val.data(), h_S_val.size() * sizeof(float), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_T_rowPtr, h_T_rowPtr.data(), h_T_rowPtr.size() * sizeof(int), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_T_colInd, h_T_colInd.data(), h_T_colInd.size() * sizeof(int), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_T_val, h_T_val.data(), h_T_val.size() * sizeof(float), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_X, h_X.data(), h_X.size() * sizeof(float), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_Y, h_Y.data(), h_Y.size() * sizeof(float), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_H, h_H.data(), h_H.size() * sizeof(float), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_K, h_K.data(), h_K.size() * sizeof(float), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_W, h_W.data(), h_W.size() * sizeof(float), cudaMemcpyHostToDevice));
    // CUDA_CHECK(cudaMemcpy(d_V, h_V.data(), h_V.size() * sizeof(int), cudaMemcpyHostToDevice));
    //
    // // --- Invocazione Pipeline ---
    // std::cout << "Esecuzione della pipeline estesa da codice C++..." << std::endl;
    // launch_prln_step_wrapper(
    //     d_S_rowPtr, d_S_colInd, d_S_val,
    //     d_T_rowPtr, d_T_colInd, d_T_val,
    //     d_X, d_Y, d_H, d_K, d_W, d_V,
    //     d_Z, d_X_prime, d_Y_prime,
    //     d_C, d_incdec,
    //     alpha, theta, num_rows, num_cols
    // );
    //
    // // --- Recupero dei Risultati ---
    // CUDA_CHECK(cudaMemcpy(h_Z.data(), d_Z, h_Z.size() * sizeof(float), cudaMemcpyDeviceToHost));
    // CUDA_CHECK(cudaMemcpy(h_X_prime.data(), d_X_prime, h_X_prime.size() * sizeof(float), cudaMemcpyDeviceToHost));
    // CUDA_CHECK(cudaMemcpy(h_Y_prime.data(), d_Y_prime, h_Y_prime.size() * sizeof(float), cudaMemcpyDeviceToHost));
    // CUDA_CHECK(cudaMemcpy(h_C.data(), d_C, h_C.size() * sizeof(float), cudaMemcpyDeviceToHost));
    // CUDA_CHECK(cudaMemcpy(&h_incdec, d_incdec, sizeof(int), cudaMemcpyDeviceToHost));
    //
    // // --- Stampa dei Risultati ---
    // std::cout << "\n--- RISULTATI COMPLETI ---" << std::endl;
    // for(int i = 0; i < num_rows; ++i) std::cout << "Z[" << i << "] = " << h_Z[i] << std::endl;
    // std::cout << std::endl;
    // for(int i = 0; i < num_cols; ++i) std::cout << "X'[" << i << "] = " << h_X_prime[i] << std::endl;
    // std::cout << std::endl;
    // for(int i = 0; i < num_cols; ++i) std::cout << "Y'[" << i << "] = " << h_Y_prime[i] << std::endl;
    // std::cout << std::endl;
    // for(int i = 0; i < dim_C; ++i)   std::cout << "C[" << i << "] = " << h_C[i] << std::endl;
    //
    // std::cout << "\n>>> incdec = " << h_incdec << " <<<" << std::endl;
    //
    // // --- Pulizia Memoria Device ---
    // cudaFree(d_S_rowPtr); cudaFree(d_S_colInd); cudaFree(d_S_val);
    // cudaFree(d_T_rowPtr); cudaFree(d_T_colInd); cudaFree(d_T_val);
    // cudaFree(d_X); cudaFree(d_Y); cudaFree(d_H); cudaFree(d_K); cudaFree(d_W); cudaFree(d_V);
    // cudaFree(d_Z); cudaFree(d_X_prime); cudaFree(d_Y_prime); cudaFree(d_C); cudaFree(d_incdec);
// ---- CONFIGURAZIONE FUNZIONE 1 ----
    const int num_rows = 4; const int num_cols = 4; const int num_rows_M = 2;
    const float alpha = 0.5f; const float theta = 0.2f;

    std::vector<int> h_S_rowPtr = {0, 1, 2, 3, 4}; std::vector<int> h_S_colInd = {0, 1, 2, 3}; std::vector<float> h_S_val = {1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<int> h_T_rowPtr = {0, 1, 2, 3, 4}; std::vector<int> h_T_colInd = {0, 1, 2, 3}; std::vector<float> h_T_val = {1.0f, 1.0f, 1.0f, 1.0f};

    // Matrice sparsa M [2 x 4]
    std::vector<int> h_M_rowPtr = {0, 2, 4}; std::vector<int> h_M_colInd = {0, 1, 2, 3}; std::vector<float> h_M_val = {0.5f, 0.5f, 1.0f, 1.0f};

    std::vector<float> h_X = {0.5f, 0.1f, 0.6f, 0.7f}; std::vector<float> h_Y = {0.8f, 0.8f, 0.8f, 0.8f};
    std::vector<float> h_H = {0.2f, 0.2f, 0.2f, 0.2f}; std::vector<float> h_K = {0.1f, 0.1f, 0.1f, 0.1f};
    std::vector<float> h_W = {2.0f, 2.0f, 2.0f, 2.0f}; std::vector<float> h_N = {0.1f, 0.3f};
    std::vector<int>   h_V = {0, 0, 1, 2};

    std::vector<float> h_Z(num_rows); std::vector<float> h_X_prime(num_cols); std::vector<float> h_Y_prime(num_cols);
    std::vector<float> h_C(num_cols / 2); std::vector<float> h_I(num_rows_M); float h_incdec = 0.0f;

    int *d_S_rowPtr, *d_S_colInd, *d_T_rowPtr, *d_T_colInd, *d_M_rowPtr, *d_M_colInd, *d_V;
    float *d_S_val, *d_T_val, *d_M_val, *d_X, *d_Y, *d_H, *d_K, *d_W, *d_N, *d_Z, *d_X_prime, *d_Y_prime, *d_C, *d_I, *d_incdec;

    CUDA_CHECK(cudaMalloc(&d_S_rowPtr, h_S_rowPtr.size() * sizeof(int))); CUDA_CHECK(cudaMalloc(&d_S_colInd, h_S_colInd.size() * sizeof(int))); CUDA_CHECK(cudaMalloc(&d_S_val, h_S_val.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_T_rowPtr, h_T_rowPtr.size() * sizeof(int))); CUDA_CHECK(cudaMalloc(&d_T_colInd, h_T_colInd.size() * sizeof(int))); CUDA_CHECK(cudaMalloc(&d_T_val, h_T_val.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_M_rowPtr, h_M_rowPtr.size() * sizeof(int))); CUDA_CHECK(cudaMalloc(&d_M_colInd, h_M_colInd.size() * sizeof(int))); CUDA_CHECK(cudaMalloc(&d_M_val, h_M_val.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_X, h_X.size() * sizeof(float))); CUDA_CHECK(cudaMalloc(&d_Y, h_Y.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_H, h_H.size() * sizeof(float))); CUDA_CHECK(cudaMalloc(&d_K, h_K.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_W, h_W.size() * sizeof(float))); CUDA_CHECK(cudaMalloc(&d_N, h_N.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_V, h_V.size() * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_Z, h_Z.size() * sizeof(float))); CUDA_CHECK(cudaMalloc(&d_X_prime, h_X_prime.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_Y_prime, h_Y_prime.size() * sizeof(float))); CUDA_CHECK(cudaMalloc(&d_C, h_C.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_I, h_I.size() * sizeof(float))); CUDA_CHECK(cudaMalloc(&d_incdec, sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_S_rowPtr, h_S_rowPtr.data(), h_S_rowPtr.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_S_colInd, h_S_colInd.data(), h_S_colInd.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_S_val, h_S_val.data(), h_S_val.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_T_rowPtr, h_T_rowPtr.data(), h_T_rowPtr.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_T_colInd, h_T_colInd.data(), h_T_colInd.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_T_val, h_T_val.data(), h_T_val.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_M_rowPtr, h_M_rowPtr.data(), h_M_rowPtr.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_M_colInd, h_M_colInd.data(), h_M_colInd.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_M_val, h_M_val.data(), h_M_val.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_X, h_X.data(), h_X.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_Y, h_Y.data(), h_Y.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_H, h_H.data(), h_H.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_K, h_K.data(), h_K.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_W, h_W.data(), h_W.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_N, h_N.data(), h_N.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_V, h_V.data(), h_V.size() * sizeof(int), cudaMemcpyHostToDevice));

    launch_prln_step_wrapper(
        d_S_rowPtr, d_S_colInd, d_S_val, d_T_rowPtr, d_T_colInd, d_T_val, d_M_rowPtr, d_M_colInd, d_M_val,
        d_X, d_Y, d_H, d_K, d_W, d_V, d_N, d_Z, d_X_prime, d_Y_prime, d_C, d_I, d_incdec,
        alpha, theta, num_rows, num_cols, num_rows_M
    );

    CUDA_CHECK(cudaMemcpy(h_Z.data(), d_Z, h_Z.size() * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&h_incdec, d_incdec, sizeof(float), cudaMemcpyDeviceToHost));

    std::cout << "--- FUNZIONE 1: TEST SUCCESSFUL ---" << std::endl;
    std::cout << "Z[0] = " << h_Z[0] << " | incdec (Float totale) = " << h_incdec << std::endl;

    // ---- CONFIGURAZIONE FUNZIONE 2 ----
    const int iter_size = 4; const int max_iters = 10;
    std::vector<int> h_R_rowPtr = {0, 1, 2, 3, 4}; std::vector<int> h_R_colInd = {1, 2, 3, 0}; std::vector<float> h_R_val = {1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<float> h_Y_iter = {0.5f, 0.1f, 0.0f, 0.6f}; std::vector<float> h_G(iter_size, 0.0f);

    int *d_R_rowPtr, *d_R_colInd; float *d_R_val, *d_Y_iter, *d_G;
    CUDA_CHECK(cudaMalloc(&d_R_rowPtr, h_R_rowPtr.size() * sizeof(int))); CUDA_CHECK(cudaMalloc(&d_R_colInd, h_R_colInd.size() * sizeof(int))); CUDA_CHECK(cudaMalloc(&d_R_val, h_R_val.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_Y_iter, h_Y_iter.size() * sizeof(float))); CUDA_CHECK(cudaMalloc(&d_G, h_G.size() * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_R_rowPtr, h_R_rowPtr.data(), h_R_rowPtr.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_R_colInd, h_R_colInd.data(), h_R_colInd.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_R_val, h_R_val.data(), h_R_val.size() * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_Y_iter, h_Y_iter.data(), h_Y_iter.size() * sizeof(float), cudaMemcpyHostToDevice));

    std::cout << "\n--- FUNZIONE 2: SOLVER ITERATIVO ---" << std::endl;
    launch_iterative_inconsistency_propagation(d_R_rowPtr, d_R_colInd, d_R_val, d_Y_iter, d_G, theta, max_iters, iter_size);

    CUDA_CHECK(cudaMemcpy(h_G.data(), d_G, h_G.size() * sizeof(float), cudaMemcpyDeviceToHost));
    for (int i = 0; i < iter_size; ++i) std::cout << "G[" << i << "] = " << h_G[i] << std::endl;

    // Pulizia finale
    cudaFree(d_S_rowPtr); cudaFree(d_S_colInd); cudaFree(d_S_val); cudaFree(d_T_rowPtr); cudaFree(d_T_colInd); cudaFree(d_T_val);
    cudaFree(d_M_rowPtr); cudaFree(d_M_colInd); cudaFree(d_M_val); cudaFree(d_X); cudaFree(d_Y); cudaFree(d_H); cudaFree(d_K);
    cudaFree(d_W); cudaFree(d_N); cudaFree(d_V); cudaFree(d_Z); cudaFree(d_X_prime); cudaFree(d_Y_prime); cudaFree(d_C); cudaFree(d_I); cudaFree(d_incdec);
    cudaFree(d_R_rowPtr); cudaFree(d_R_colInd); cudaFree(d_R_val); cudaFree(d_Y_iter); cudaFree(d_G);


    // Configurazione MDD: 4 variabili, dominio ternario {0, 1, 2}
    int num_vars = 4;
    int domain_size = 3;

    CUDAMddCOO mdd(num_vars, domain_size);

    std::vector<int> target = {1, 2, 0, 1};

    // 2) Verifica iniziale di presenza nel grafo denso
    std::cout << "Iniziale ammissibilita': " << mdd.isAdmissible(target) << " (Atteso: 1)\n";

    // Ispezione coordinate tramite l'accessore COO simulato
    COOTuple coo_info = mdd.getCOOAccessor(target);
    std::cout << "[COO Accessor] -> Row (Parent): " << coo_info.row
              << " | Col (Terminal): " << coo_info.col
              << " | Value: " << coo_info.value
              << " | Active: " << coo_info.active << "\n";

    // 3) Rimozione dell'assegnamento
    std::cout << "\nRimuovo l'assegnamento target...\n";
    mdd.removeAssignment(target);

    // 4) Verifica post-rimozione
    std::cout << "Post-rimozione ammissibilita': " << mdd.isAdmissible(target) << " (Atteso: 0)\n";

    // Controllo di un cammino adiacente (non deve essere stato modificato)
    std::vector<int> sibling = {1, 2, 0, 2};
    std::cout << "Fratello affine ammissibilita': " << mdd.isAdmissible(sibling) << " (Atteso: 1)\n";


    // Definiamo due assegnamenti in cui X_1 (la seconda variabile) è uguale a 2
    std::vector<int> path_A = {0, 2, 1, 1}; // X_1 = 2
    std::vector<int> path_B = {1, 2, 0, 0}; // X_1 = 2
    std::vector<int> path_C = {1, 0, 0, 0}; // X_1 = 0 (Non deve essere rimosso!)

    std::cout << "--- Verifiche Iniziali ---\n";
    std::cout << "Path A ammissibile: " << mdd.isAdmissible(path_A) << " (Atteso: 1)\n";
    std::cout << "Path B ammissibile: " << mdd.isAdmissible(path_B) << " (Atteso: 1)\n";
    std::cout << "Path C ammissibile: " << mdd.isAdmissible(path_C) << " (Atteso: 1)\n";

    // Eseguiamo la rimozione di TUTTI i cammini dove X_1 == 2
    std::cout << "\nRimo tutti gli assegnamenti dove la variabile X_1 e' pari a 2...\n";
    mdd.removeVariableConstant(1, 2); // target_var = 1, constant_val = 2

    std::cout << "\n--- Verifiche Post Rimozione Massiva ---\n";
    std::cout << "Path A ammissibile: " << mdd.isAdmissible(path_A) << " (Atteso: 0)\n";
    std::cout << "Path B ammissibile: " << mdd.isAdmissible(path_B) << " (Atteso: 0)\n";
    std::cout << "Path C ammissibile: " << mdd.isAdmissible(path_C) << " (Atteso: 1)\n";


    std::cout << "=== ENGINE COUPLING TEST: STRUTTURA IBRIDA CSF + DELTA ===\n";
    logds::types::SparseTensor cpu_tensor{{10,10}, 0.0};
    cpu_tensor.set_value({0, 1}, 1.5);
    cpu_tensor.set_value({1, 1}, 2.5);
    cpu_tensor.set_value({2, 2}, 3.5);
    cpu_tensor.set_value({3, 2}, 4.5);

    std::vector<uint64_t> shape = {10, 10};
    size_t cap_budget = 500;

    // 2. Allocazione e compilazione in-place dell'albero sulla GPU
    DynamicHybridTensor T_A(shape, cap_budget);
    DynamicHybridTensor T_B(shape, cap_budget);

    T_A.initialize_from_cpu(cpu_tensor);
    T_B.initialize_from_cpu(cpu_tensor);
    std::cout << " -> Albero CSF compilato con successo su GPU. NNZ stabili: " << T_A.csf_nnz << "\n";

    std::cout << "\n=== STEP ADAM (MUTAZIONE CASUALE IN-PLACE IN O(1)) ===\n";
    T_A.set_value_atomic({4, 4}, 99.9f); // Inserimento lock-free nel Delta Buffer d'appoggio

    std::cout << "\n=== STEP GENERALIZZATO N-WAY EINSUM GUIDATO DA ID ===\n";
    // Configurazione della ricetta tramite ID assi interi costanti (Esempio: A(i,j) * B(j,k) -> Out(i,k))
    EinsumRecipe recipe;
    recipe.input_axes = {
            {0, 1}, // Pesi/Gradienti di A (Assi 0 e 1)
            {1, 2}  // Pesi/Gradienti di B (Assi 1 e 2)
    };
    recipe.output_axes = {0, 2}; // Output finale (Assi 0 e 2)

    std::vector<DynamicHybridTensor*> pipeline = {&T_A, &T_B};
    DynamicHybridTensor Out = DynamicHybridTensor::n_way_einstein_sum(pipeline, recipe, shape, cap_budget);



    return 0;
}
