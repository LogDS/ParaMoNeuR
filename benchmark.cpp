
#include <cuda_runtime.h>
#include <driver_types.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"


void run_scalability_experiment(size_t current_nnz, std::ofstream& csv_file) {
    std::vector<uint64_t> shape_5d = {50, 50, 50, 20, 10};
    size_t fixed_capacity = static_cast<size_t>(current_nnz * 1.4);

    cudaEvent_t start_mut, stop_mut, start_alg, stop_alg;
    cudaEventCreate(&start_mut); cudaEventCreate(&stop_mut);
    cudaEventCreate(&start_alg); cudaEventCreate(&stop_alg);

    DynamicHybridTensor T_A(shape_5d, fixed_capacity);
    DynamicHybridTensor T_B(shape_5d, fixed_capacity);

    logds::types::SparseTensor cpu_A("benchmark_data/tensor_A_" + std::to_string(current_nnz) + ".csv",0.0 );
    logds::types::SparseTensor  cpu_B("benchmark_data/tensor_B_" + std::to_string(current_nnz) + ".csv", 0.0);

    // 1. MISURA MUTAZIONE (Inizializzazione da file paritetici)
    cudaEventRecord(start_mut);
    T_A.initialize_from_cpu(cpu_A);
    T_B.initialize_from_cpu(cpu_B);
    cudaEventRecord(stop_mut);
    cudaEventSynchronize(stop_mut);
    float t_mutation_ms = 0;
    cudaEventElapsedTime(&t_mutation_ms, start_mut, stop_mut);

    T_A.consolidate();
    T_B.consolidate();

    // 2. MISURA ALGEBRA PESANTE (EINSUM)
    cudaEventRecord(start_alg);
    EinsumRecipe recipe;
    recipe.input_axes = {{0, 1, 2, 3, 4}, {0, 1, 2, 3, 4}};
    recipe.output_axes = {0, 4};
    std::vector<DynamicHybridTensor*> nargs{&T_A, &T_B};
    DynamicHybridTensor Out_Einsum = DynamicHybridTensor::n_way_einstein_sum(nargs, recipe, {50, 10}, fixed_capacity);

    cudaEventRecord(stop_alg);
    cudaEventSynchronize(stop_alg);
    float t_algebra_ms = 0;
    cudaEventElapsedTime(&t_algebra_ms, start_alg, stop_alg);

    // --- BARRIERA DI CONTROLLO ANTI-OTTIMIZZAZIONE (DCE BLOCK) ---
    // Scarichiamo il primo float calcolato dall'Einsum verso la CPU.
    // Questo rende l'esito del kernel "necessario", distruggendo la Dead Code Elimination di nvcc!
    float h_anti_dce_verification = 0.0f;
    cudaMemcpy(&h_anti_dce_verification, Out_Einsum.d_csf_vals, sizeof(float), cudaMemcpyDeviceToHost);

    // Scrittura dei log reali ed effettivi
    csv_file << "Paramoneur," << current_nnz << "," << t_mutation_ms << "," << t_algebra_ms << "\n";
    std::cout << "[REAL RUN NNZ: " << current_nnz << "] Mutazione: " << t_mutation_ms << " ms | Algebra: " << t_algebra_ms << " ms | Controllo Hardware: " << h_anti_dce_verification << "\n";

    cudaEventDestroy(start_mut); cudaEventDestroy(stop_mut);
    cudaEventDestroy(start_alg); cudaEventDestroy(stop_alg);
}

int main() {
    int deviceCount = 0;
    cudaGetDeviceCount(&deviceCount);
    std::ofstream csv_file("benchmark_custom_engine.csv");
    csv_file << "NNZ,Tempo_Mutazione_ms,Tempo_Algebra_ms\n";

    std::vector<size_t> nnz_scale_steps = {1000, 5000, 25000, 100000, 500000};

    for (size_t current_nnz : nnz_scale_steps) {
        run_scalability_experiment(current_nnz, csv_file);
    }

    csv_file.close();
    return 0;
}
