//
// Created by gyankos on 28/10/26.
//

#include "logds/paramoneur/engine/mdd/mdd_coo.h"
#include "logds/paramoneur/engine/mdd/mdd_coo_kernel.cuh"
#include <cmath>
#include <stdexcept>

CUDAMddCOO::CUDAMddCOO(int vars, int domain) : num_vars(vars), domain_size(domain) {
    total_assignments = static_cast<long long>(std::pow(domain_size, num_vars));

    cudaMalloc(&d_mask, total_assignments * sizeof(bool));

    int blockSize = 256;
    long long numBlocks = (total_assignments + blockSize - 1) / blockSize;
    initMaskKernel<<<numBlocks, blockSize>>>(d_mask, total_assignments);
    cudaDeviceSynchronize();
}

CUDAMddCOO::~CUDAMddCOO() {
    cudaFree(d_mask);
}

long long CUDAMddCOO::getLinearIndex(const std::vector<int>& assignment) const {
    long long idx = 0;
    for (int val : assignment) {
        idx = idx * domain_size + val;
    }
    return idx;
}

void CUDAMddCOO::removeAssignment(const std::vector<int>& assignment) {
    if (assignment.size() != static_cast<size_t>(num_vars)) {
        throw std::invalid_argument("Dimensione assegnamento non valida.");
    }
    long long linear_idx = getLinearIndex(assignment);
    bool false_val = false;
    cudaMemcpy(&d_mask[linear_idx], &false_val, sizeof(bool), cudaMemcpyHostToDevice);
}

bool CUDAMddCOO::isAdmissible(const std::vector<int>& assignment) const {
    if (assignment.size() != static_cast<size_t>(num_vars)) return false;
    long long linear_idx = getLinearIndex(assignment);
    bool result;
    cudaMemcpy(&result, &d_mask[linear_idx], sizeof(bool), cudaMemcpyDeviceToHost);
    return result;
}

std::vector<uint8_t> CUDAMddCOO::isAdmissibleBatch(const std::vector<int> &h_queries, int num_queries) {
    int queries_size = num_queries * num_vars;
    int* d_queries;
    bool* d_results; // Sulla GPU 'bool' occupa 1 byte nativo, compatibile con uint8_t in C++
    std::vector<uint8_t> h_results(num_queries);

    cudaMalloc(&d_queries, queries_size * sizeof(int));
    cudaMalloc(&d_results, num_queries * sizeof(bool)); // In CUDA, sizeof(bool) == 1

    cudaMemcpy(d_queries, h_queries.data(), queries_size * sizeof(int), cudaMemcpyHostToDevice);

    int blockSize = 256;
    int numBlocks = (num_queries + blockSize - 1) / blockSize;
    queryBatchKernel<<<numBlocks, blockSize>>>(d_queries, d_mask, d_results, num_vars, domain_size, num_queries);
    cudaDeviceSynchronize();

    // Copia diretta e sicura grazie alla contiguità di uint8_t (1 byte) e il bool di CUDA (1 byte)
    cudaMemcpy(h_results.data(), d_results, num_queries * sizeof(bool), cudaMemcpyDeviceToHost);

    cudaFree(d_queries);
    cudaFree(d_results);

    return h_results;
}

COOTuple CUDAMddCOO::getCOOAccessor(const std::vector<int>& assignment) const {
    long long linear_idx = getLinearIndex(assignment);
    bool active = isAdmissible(assignment);

    long long parent_node_id = linear_idx / domain_size;
    int edge_value = assignment.back();

    return COOTuple{ parent_node_id, 1, edge_value + 1, active };
}

void CUDAMddCOO::removeVariableConstant(int target_var, int constant_val) {
    if (target_var < 0 || target_var >= num_vars) {
        throw std::invalid_argument("Indice variabile target fuori dai limiti.");
    }
    if (constant_val < 0 || constant_val >= domain_size) {
        throw std::invalid_argument("Valore della costante fuori dal dominio.");
    }

    int blockSize = 256;
    long long numBlocks = (total_assignments + blockSize - 1) / blockSize;

    // Lancio del kernel globale sulla GPU
    removeVariableConstantKernel<<<numBlocks, blockSize>>>(d_mask, total_assignments,
                                                            num_vars, domain_size,
                                                            target_var, constant_val);

    // Sincronizziamo il device per assicurarci che i cammini siano invalidati prima delle prossime query
    cudaDeviceSynchronize();
}