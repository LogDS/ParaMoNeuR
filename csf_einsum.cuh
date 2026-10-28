#ifndef CSF_EINSUM_CUH
#define CSF_EINSUM_CUH

#include <cuda_runtime.h>
#include <string>

#define MAX_TENSOR_ORDER 7

// Contiene i metadati dinamici estratti dal parser host
struct EinsumPlan {
    int tensor_order;
    int num_contracted_modes;
    int contracted_levels[MAX_TENSOR_ORDER];
    int dense_dim_strides[MAX_TENSOR_ORDER];
};

// Struttura CSF Universale robusta per il multiprocessing parallelo
struct UniversalCSF {
    // int order;
    // int nnz;
    // float* d_vals;           // Valori float delle foglie (Dimensione: nnz)
    //
    // // Matrice delle coordinate [nnz * order] allocata sul device.
    // // Ogni riga contiene le coordinate esatte (i, j, k, o...) di quella foglia.
    // int* d_coordinate_matrix;

    int order;
    int nnz;

    // Unico puntatore alla memoria globale allocata sul device
    void* d_unified_buffer;
    size_t total_buffer_bytes;

    // Puntatori derivati tramite aritmetica degli offset (puntano all'interno di d_unified_buffer)
    float* d_vals;
    int* d_coordinate_matrix;
};



// NUOVA STRUTTURA: Buffer Unificato per il Risultato COO (Output)
struct UnifiedCOOOutput {
    int out_order;
    int max_nnz;
    void* d_unified_buffer;
    size_t total_buffer_bytes;

    // Puntatori derivati tramite aritmetica degli offset (dentro d_unified_buffer)
    int* d_out_indices;     // Array bidimensionale di indici [max_nnz * out_order]
    float* d_out_vals;      // Array di valori float [max_nnz]
    int* d_global_counter;  // Contatore atomico hardware per gli elementi validi finali
};



// Interfaccia di lancio modificata per accettare la nuova struttura di output
void launch_csf_einsum(
    const UniversalCSF& csf,
    const EinsumPlan& plan,
    const float* d_dense_input,
    UnifiedCOOOutput& out
);


// Struttura per il caricamento dei dati COO sparsi e disordinati dall'Host
struct RawCOOElement {
    int coords[MAX_TENSOR_ORDER];
    float val;
};


#endif // CSF_EINSUM_CUH
