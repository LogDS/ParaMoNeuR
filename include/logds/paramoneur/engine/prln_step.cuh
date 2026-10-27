//
// Created by gyankos on 27/10/26.
//

#ifndef PARAMONEUR2_PRLN_STEP_H
#define PARAMONEUR2_PRLN_STEP_H


void launch_prln_step_wrapper(
    int* d_S_rowPtr, int* d_S_colInd, float* d_S_val,
    int* d_T_rowPtr, int* d_T_colInd, float* d_T_val,
    float* d_X, float* d_Y, float* d_H, float* d_K, float* d_W, int* d_V,
    float* d_Z, float* d_X_prime, float* d_Y_prime,
    float* d_C, int* d_incdec, // Nuovi output
    float alpha, float theta, int num_rows, int num_cols
);


#endif //PARAMONEUR2_PRLN_STEP_H
