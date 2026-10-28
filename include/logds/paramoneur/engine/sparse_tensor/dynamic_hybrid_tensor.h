//
// Created by gyankos on 28/10/26.
//

#ifndef PARAMONEUR2_SPARSE_TENSOR_H
#define PARAMONEUR2_SPARSE_TENSOR_H

#include <vector>
#include <cstdint>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/device_vector.h>
#include <thrust/sort.h>
#include <thrust/execution_policy.h>
// #include <cub/cub.hpp>
#include <stdint.h>
#include <vector>

#include "logds/paramoneur/types/SparseTensor.h"

#ifdef NOT_WORKING_VERSION
struct EinsumRecipe {
    std::vector<std::vector<int>> input_axes;
    std::vector<int> output_axes;
};

enum class TensorState { HYBRID_ACTIVE, CONSOLIDATED_CSF };
enum class ReductionOp { SUM, MAX, MIN, PROD };

class DynamicHybridTensor {
public:
    size_t num_dims;
    std::vector<uint64_t> dims;
    size_t capacity;
    TensorState state;

    // --- Contatori Hardware (GPU) ---
    uint32_t* d_csf_nnz;
    uint32_t* d_delta_nnz;

    // --- Array CSF Principale (GPU) ---
    std::vector<uint64_t*> d_fptr;
    std::vector<uint64_t*> d_fids;
    float* d_csf_vals;

    // --- Array COO Delta Buffer per Adam (GPU) ---
    uint64_t** d_delta_coords;
    float* d_delta_vals;

    DynamicHybridTensor(const std::vector<uint64_t>& input_dims, size_t cap);
    ~DynamicHybridTensor();

    void clear();
    void initialize_from_cpu(const logds::types::SparseTensor& cpu_tensor);
    void set_value_atomic(const std::vector<uint64_t>& coords, float value);
    void consolidate();

    void hadamard(DynamicHybridTensor& B);
    void cell_sum(DynamicHybridTensor& B);
    void aggregate_axes(const std::vector<uint8_t>& keep_axes, ReductionOp op);

    static DynamicHybridTensor n_way_einstein_sum(
        std::vector<DynamicHybridTensor*>& inputs,
        const EinsumRecipe& recipe,
        const std::vector<uint64_t>& out_dims,
        size_t out_capacity
    );
};

#else


#define MAX_ORDER 7

// Struttura fissa per le coordinate di un singolo elemento.
// Ad ogni elemento dei values corrisponderà esattamente una di queste tuple.
struct IndexTuple {
    int idx[MAX_ORDER];
};

// Comparatore GPU per l'ordinamento lessicografico delle tuple di indici
struct LexicographicalLessThan {
    int order;

    __host__ __device__ LexicographicalLessThan(int o) : order(o) {}

    __host__ __device__ bool operator()(const IndexTuple& a, const IndexTuple& b) const {
        for (int i = 0; i < order; ++i) {
            if (a.idx[i] < b.idx[i]) return true;
            if (a.idx[i] > b.idx[i]) return false;
        }
        return false; // Sono uguali
    }
};

// Struttura COO aggiornata lato Host
struct HostCOO {
    int order;
    int dims[MAX_ORDER];
    int nnz;
    std::vector<IndexTuple> coords; // Dimensione: [nnz], ad ogni elemento corrisponde un indice composto
    std::vector<float> values;      // Dimensione: [nnz]
};

// Livello dell'albero CSF sul Device
struct CSFLevelDevice {
    int* f_ptr;
    int* f_crd;
    int num_nodes;
};

enum IndexMappingType {
    AXIS_IN_BOTH = 0,   // L'asse è presente sia in A che in B
    AXIS_ONLY_A  = 1,   // L'asse è presente solo in A
    AXIS_ONLY_B  = 2    // L'asse è presente solo in B
};

// Metadati di avanzamento per ogni singolo livello dell'iteratore di einsum
struct AxisStep {
    IndexMappingType map_type;
    int level_in_A;     // Indice del livello in A (-1 se non presente)
    int level_in_B;     // Indice del livello in B (-1 se non presente)
    int out_axis_pos;   // Posizione di questo asse nel tensore di output (-1 se ridotto/contratto)
};

struct EinsumPlan {
    int total_logical_axes;        // Numero totale di assi logici unici elaborati
    int out_order;                  // Ordine (N) del tensore di output
    int out_dims[MAX_ORDER];        // Dimensioni delle modalità di output
    AxisStep steps[MAX_ORDER * 2];  // Sequenza di scansione degli assi logici
};

struct CSFLevel {
    int* f_ptr;  // Array dei puntatori alla fibra del livello successivo
    int* f_crd;  // Array delle coordinate reali di questo livello
};

struct COOElement {
    int coords[MAX_ORDER];
    float value;
};

#endif


#endif //PARAMONEUR2_SPARSE_TENSOR_H
