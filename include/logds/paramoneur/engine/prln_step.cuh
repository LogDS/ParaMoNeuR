//
// Created by gyankos on 27/10/26.
//

#ifndef PARAMONEUR2_PRLN_STEP_H
#define PARAMONEUR2_PRLN_STEP_H


void launch_prln_step_wrapper(
int* d_S_rowPtr, int* d_S_colInd, float* d_S_val,
int* d_T_rowPtr, int* d_T_colInd, float* d_T_val,
int* d_M_rowPtr, int* d_M_colInd, float* d_M_val, // Matrice M
float* d_X, float* d_Y, float* d_H, float* d_K, float* d_W, int* d_V, float* d_N,
float* d_Z, float* d_X_prime, float* d_Y_prime, float* d_C, float* d_I,
float* d_incdec, // Ora è un float*
float alpha, float theta, int num_rows, int num_cols, int num_rows_M
);

void launch_iterative_inconsistency_propagation(
    int* d_R_rowPtr, int* d_R_colInd, float* d_R_val, // Matrice quadrata R
    float* d_Y, float* d_G,                           // Input Y e Output G
    float theta, int maxiterations, int size
);

#endif //PARAMONEUR2_PRLN_STEP_H
