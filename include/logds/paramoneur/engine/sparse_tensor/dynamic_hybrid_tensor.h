// // // //
// // // // Created by gyankos on 28/10/26.
// // // //
// // //
// // // #ifndef PARAMONEUR2_SPARSE_TENSOR_H
// // // #define PARAMONEUR2_SPARSE_TENSOR_H
// // //
// // // #include <vector>
// // // #include <cstdint>
// // // #include <cuda_runtime.h>
// // // #include <device_launch_parameters.h>
// // // #include <thrust/device_vector.h>
// // // #include <thrust/sort.h>
// // // #include <thrust/execution_policy.h>
// // // // #include <cub/cub.hpp>
// // // #include <stdint.h>
// // // #include <vector>
// // //
// // // #include "logds/paramoneur/types/SparseTensor.h"
// // //
// // // #ifdef NOT_WORKING_VERSION
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
// // //
// // // #else
// // //
// // //
// // // #define MAX_ORDER 7
// // //
// // // // Struttura fissa per le coordinate di un singolo elemento.
// // // // Ad ogni elemento dei values corrisponderà esattamente una di queste tuple.
// // // struct IndexTuple {
// // //     int idx[MAX_ORDER];
// // // };
// // //
// // // // Comparatore GPU per l'ordinamento lessicografico delle tuple di indici
// // // struct LexicographicalLessThan {
// // //     int order;
// // //
// // //     __host__ __device__ LexicographicalLessThan(int o) : order(o) {}
// // //
// // //     __host__ __device__ bool operator()(const IndexTuple& a, const IndexTuple& b) const {
// // //         for (int i = 0; i < order; ++i) {
// // //             if (a.idx[i] < b.idx[i]) return true;
// // //             if (a.idx[i] > b.idx[i]) return false;
// // //         }
// // //         return false; // Sono uguali
// // //     }
// // // };
// // //
// // // // Struttura COO aggiornata lato Host
// // // struct HostCOO {
// // //     int order;
// // //     int dims[MAX_ORDER];
// // //     int nnz;
// // //     std::vector<IndexTuple> coords; // Dimensione: [nnz], ad ogni elemento corrisponde un indice composto
// // //     std::vector<float> values;      // Dimensione: [nnz]
// // // };
// // //
// // // // Livello dell'albero CSF sul Device
// // // struct CSFLevelDevice {
// // //     int* f_ptr;
// // //     int* f_crd;
// // //     int num_nodes;
// // // };
// // //
// // // enum IndexMappingType {
// // //     AXIS_IN_BOTH = 0,   // L'asse è presente sia in A che in B
// // //     AXIS_ONLY_A  = 1,   // L'asse è presente solo in A
// // //     AXIS_ONLY_B  = 2    // L'asse è presente solo in B
// // // };
// // //
// // // // Metadati di avanzamento per ogni singolo livello dell'iteratore di einsum
// // // struct AxisStep {
// // //     IndexMappingType map_type;
// // //     int level_in_A;     // Indice del livello in A (-1 se non presente)
// // //     int level_in_B;     // Indice del livello in B (-1 se non presente)
// // //     int out_axis_pos;   // Posizione di questo asse nel tensore di output (-1 se ridotto/contratto)
// // // };
// // //
// // // struct EinsumPlan {
// // //     int total_logical_axes;        // Numero totale di assi logici unici elaborati
// // //     int out_order;                  // Ordine (N) del tensore di output
// // //     int out_dims[MAX_ORDER];        // Dimensioni delle modalità di output
// // //     AxisStep steps[MAX_ORDER * 2];  // Sequenza di scansione degli assi logici
// // // };
// // //
// // // struct CSFLevel {
// // //     int* f_ptr;  // Array dei puntatori alla fibra del livello successivo
// // //     int* f_crd;  // Array delle coordinate reali di questo livello
// // // };
// // //
// // // struct COOElement {
// // //     int coords[MAX_ORDER];
// // //     float value;
// // // };
// // //
// // // // --- FIRMA DEL WRAPPER HOST ---
// // // // Questa funzione sarà l'unico punto di contatto per il file .cpp
// // // void launch_csf_einsum_wrapper(
// // //     const CSFLevel* d_A_levels, const float* d_vals_A, int A_root_size,
// // //     const CSFLevel* d_B_levels, const float* d_vals_B, int B_root_size,
// // //     EinsumPlan plan,
// // //     COOElement* d_coo_workspace,
// // //     int* d_global_coo_counter,
// // //     int max_workspace_size
// // // );
// // //
// // // void load_coo_host_to_csf_device(const HostCOO& host_coo, CSFLevelDevice* h_levels, float*& d_final_vals);
// // //
// // // void finalize_einsum_to_csr(
// // //     COOElement* d_coo_workspace, int nnz_generated,
// // //     int out_rows, int out_cols, int* d_out_dims,
// // //     int*& d_csr_row_ptr, int*& d_csr_col_ind, float*& d_csr_vals, int& final_nnz
// // // );
// // //
// // // #endif
// // //
// // //
// // // #endif //PARAMONEUR2_SPARSE_TENSOR_H
// //
// // #ifndef PARAMONEUR2_SPARSE_TENSOR_H
// // #define PARAMONEUR2_SPARSE_TENSOR_H
// //
// // #include <vector>
// // #include <cstdint>
// //
// // #define MAX_ORDER 7
// //
// // // Struttura fissa per le coordinate di un singolo elemento.
// // // Assicura l'accoppiamento nativo 1:1 tra valori e indici multidimensionali.
// // struct IndexTuple {
// //     int idx[MAX_ORDER];
// // };
// //
// //
// //
// // // Struttura dati COO sul lato Host per il caricamento dei dati
// // struct HostCOO {
// //     int order;
// //     int dims[MAX_ORDER];
// //     int nnz;
// //     std::vector<IndexTuple> coords; // Dimensione: [nnz]
// //     std::vector<float> values;      // Dimensione: [nnz]
// // };
// //
// // // Struttura utilizzata per gestire i metadati di allocazione lato Host
// // struct CSFLevelDevice {
// //     int* f_ptr;
// //     int* f_crd;
// //     int num_nodes;
// // };
// //
// // // Tipi di mappatura geometrica per singolo asse logico nell'Einsum
// // enum IndexMappingType {
// //     AXIS_IN_BOTH = 0,   // L'asse è presente sia nel tensore A che nel tensore B
// //     AXIS_ONLY_A  = 1,   // L'asse è presente esclusivamente nel tensore A
// //     AXIS_ONLY_B  = 2    // L'asse è presente esclusivamente nel tensore B
// // };
// //
// // // Configurazione di un singolo livello logico dell'esploratore Einsum
// // struct AxisStep {
// //     IndexMappingType map_type;
// //     int level_in_A;     // Indice del livello nel formato CSF di A (-1 se assente)
// //     int level_in_B;     // Indice del livello nel formato CSF di B (-1 se assente)
// //     int out_axis_pos;   // Posizione nell'asse di output (-1 se asse contratto)
// // };
// //
// // // Piano di computazione dinamico passato al kernel
// // struct EinsumPlan {
// //     int total_logical_axes;        // Numero totale di assi unici analizzati
// //     int out_order;                  // Ordine (N) del tensore di output
// //     int out_dims[MAX_ORDER];        // Dimensioni delle modalità di output
// //     AxisStep steps[MAX_ORDER * 2];  // Sequenza di scansione e discesa
// // };
// //
// // // Struttura interna dell'albero passata ai registri di lettura del kernel CUDA
// // struct CSFLevel {
// //     int* f_ptr;  // Puntatori alla fibra del livello successivo
// //     int* f_crd;  // Coordinate reali del livello corrente
// // };
// //
// // // Elemento grezzo inserito nel workspace lineare di accumulo
// // struct COOElement {
// //     int coords[MAX_ORDER];
// //     float value;
// // };
// //
// // // --- WRAPPER E FUNZIONI DI INTERFACCIA COMPATIBILI CXX ---
// //
// // void launch_csf_einsum_wrapper(
// //     const CSFLevel* d_A_levels, const float* d_vals_A, int A_root_size,
// //     const CSFLevel* d_B_levels, const float* d_vals_B, int B_root_size,
// //     EinsumPlan plan,
// //     COOElement* d_coo_workspace,
// //     int* d_global_coo_counter,
// //     int max_workspace_size
// // );
// //
// // void load_coo_host_to_csf_device(const HostCOO& host_coo, CSFLevelDevice* h_levels, float*& d_final_vals);
// //
// // void finalize_einsum_to_csr(
// //     COOElement* d_coo_workspace, int nnz_generated,
// //     int out_rows, int out_cols, int* d_out_dims,
// //     int*& d_csr_row_ptr, int*& d_csr_col_ind, float*& d_csr_vals, int& final_nnz
// // );
// //
// // #endif //PARAMONEUR2_SPARSE_TENSOR_H
//
//
// #ifndef CSF_EINSUM_CUH
// #define CSF_EINSUM_CUH
//
// #include <cuda_runtime.h>
//
// // Limite massimo del rango del tensore imposto dai vincoli strutturali
// #define MAX_TENSOR_ORDER 7
//
// // Struttura host/device per il piano di contrazione Einsum tramite array statici
// struct EinsumPlan {
//     int tensor_order;
//     int num_contracted_modes;
//     int contracted_levels[MAX_TENSOR_ORDER];
//     int dense_dim_strides[MAX_TENSOR_ORDER];
// };
//
// // Rappresentazione linearizzata e statica del CSF sul Device
// // struct UniversalCSF {
// //     int order;
// //     int nnz;
// //     int* d_all_ptrs;         // Buffer globale concatenato dei puntatori
// //     int* d_all_indices;      // Buffer globale concatenato degli indici
// //     float* d_vals;           // Array dei valori float (foglie)
// //
// //     int ptr_offsets[MAX_TENSOR_ORDER]; // Offset statici per navigare d_all_ptrs
// //     int ind_offsets[MAX_TENSOR_ORDER]; // Offset statici per navigare d_all_indices
// // };
//
// struct UniversalCSF {
//     int order;
//     int nnz;
//     int* d_all_ptrs;
//     int* d_all_indices;
//     float* d_vals;
//
//     int ptr_offsets[MAX_TENSOR_ORDER];
//     int ind_offsets[MAX_TENSOR_ORDER];
//
//     // Array bidimensionale esplicito [num_final_fibers * (order - 1)] che contiene
//     // le coordinate antenate esatte di ciascuna fibra finale per eliminare errori di arrotondamento.
//     int* d_coordinate_map;
// };
//
// // Interfaccia C++ per l'esecuzione del Kernel CUDA
// void launch_csf_einsum(
//     const UniversalCSF& csf,
//     const EinsumPlan& plan,
//     const float* d_dense_input,
//     int root_size,
//     int* d_out_indices,
//     float* d_out_vals,
//     int* d_global_counter
// );
//
// #endif // CSF_EINSUM_CUH

#ifndef CSF_EINSUM_CUH
#define CSF_EINSUM_CUH

#include <cuda_runtime.h>
#include <string>
#include <vector>

#define MAX_TENSOR_ORDER 7

// Piano di contrazione dinamico estratto dal parser
struct EinsumPlan {
    int tensor_order;
    int num_contracted_modes;
    int contracted_levels[MAX_TENSOR_ORDER];
    int dense_dim_strides[MAX_TENSOR_ORDER];
};

// Struttura CSF Universale con array statici di offset
struct UniversalCSF {
    int order;
    int nnz;
    int* d_all_ptrs;
    int* d_all_indices;
    float* d_vals;
    int ptr_offsets[MAX_TENSOR_ORDER];
    int ind_offsets[MAX_TENSOR_ORDER];
};

// Primitive Host
// EinsumPlan compile_einsum_string(const std::string& expression, const int* dense_sizes);

void launch_csf_einsum(
    const UniversalCSF& csf,
    const EinsumPlan& plan,
    const float* d_dense_input,
    int root_size,
    int* d_out_indices,
    float* d_out_vals,
    int* d_global_counter
);

#endif // CSF_EINSUM_CUH
