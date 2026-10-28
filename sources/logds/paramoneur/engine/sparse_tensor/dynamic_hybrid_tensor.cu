//
// Created by gyankos on 28/10/26.
//

#include "logds/paramoneur/engine/sparse_tensor/dynamic_hybrid_tensor.h"

#include <cuda_runtime.h>
#include <thrust/sort.h>
#include <thrust/sequence.h>
#include <thrust/gather.h>
#include <thrust/reduce.h>
#include <thrust/device_ptr.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <iostream>
#include <set>

#ifdef NOT_WORKING_VERSION

// Link esterni a tutti i kernel di kernels.cu
__global__ void delta_insert_kernel(uint64_t** delta_coords, float* delta_vals, uint32_t* delta_nnz, size_t num_dims, const uint64_t* coords, float value, size_t capacity);
__global__ void mask_axes_kernel(const uint64_t* src_keys, uint64_t* dst_keys, size_t nnz, const uint64_t* strides, const bool* keep_axes, size_t num_dims);
// All'inizio di sparse_tensor.cpp, assicurati che il kernel sia dichiarato:
__global__ void einstein_contraction_binary_kernel(const float* vals_A, const uint64_t* fids_A_dim0, size_t nnz_A, const float* vals_B, const uint64_t* fids_B_dim0, size_t nnz_B, float* vals_Out, uint64_t* fids_Out_dim0, uint32_t* out_nnz);
__global__ void coo_hadamard_kernel(const uint64_t* keys_A, float* vals_A, size_t nnz_A, const uint64_t* keys_B, const float* vals_B, size_t nnz_B);
__global__ void coo_cell_sum_kernel(float* vals_A, const float* vals_B, size_t nnz);


// Funtori per le riduzioni parallele multi-asse di Thrust
struct OpMax { __host__ __device__ float operator()(const float& a, const float& b) const { return (a > b) ? a : b; } };
struct OpMin { __host__ __device__ float operator()(const float& a, const float& b) const { return (a < b) ? a : b; } };
struct OpProd { __host__ __device__ float operator()(const float& a, const float& b) const { return a * b; } };
// Sostituisci o aggiorna la struct PermutationComparator all'inizio di sparse_tensor.cpp:
struct PermutationComparator {
    const uint64_t* d_keys;
    size_t num_dimensions;

    PermutationComparator(const uint64_t* keys, size_t dims)
        : d_keys(keys), num_dimensions(dims) {}

    __host__ __device__ bool operator()(const uint32_t& i, const uint32_t& j) const {
        // Confronta l'Asse 0 dell'elemento i-esimo con l'Asse 0 dell'elemento j-esimo
        // L'indice lineare nella matrice piatta è dato da: (indice_elemento * num_dimensioni) + asse
        uint64_t key_i = d_keys[i * num_dimensions + 0];
        uint64_t key_j = d_keys[j * num_dimensions + 0];

        if (key_i != key_j) {
            return key_i < key_j;
        }
        // Fallback deterministico sugli assi successivi in caso di parità sull'Asse 0
        return i < j;
    }
};

DynamicHybridTensor::DynamicHybridTensor(const std::vector<uint64_t>& input_dims, size_t cap)
    : dims(input_dims), capacity(cap), state(TensorState::HYBRID_ACTIVE) {
    num_dims = dims.size();
    d_fptr.resize(num_dims, nullptr);
    d_fids.resize(num_dims, nullptr);

    cudaMalloc(&d_csf_nnz, sizeof(uint32_t));
    cudaMalloc(&d_delta_nnz, sizeof(uint32_t));
    cudaMalloc(&d_csf_vals, capacity * sizeof(float));
    cudaMalloc(&d_delta_vals, capacity * sizeof(float));

    std::vector<uint64_t*> h_delta_ptrs(num_dims);
    for (size_t d = 0; d < num_dims; ++d) {
        cudaMalloc(&h_delta_ptrs[d], capacity * sizeof(uint64_t));
        cudaMalloc(&d_fids[d], capacity * sizeof(uint64_t));
        cudaMalloc(&d_fptr[d], (capacity + 1) * sizeof(uint64_t));
    }
    cudaMalloc(&d_delta_coords, num_dims * sizeof(uint64_t*));
    cudaMemcpy(d_delta_coords, h_delta_ptrs.data(), num_dims * sizeof(uint64_t*), cudaMemcpyHostToDevice);

    clear();
}

DynamicHybridTensor::~DynamicHybridTensor() {
    std::vector<uint64_t*> h_delta_ptrs(num_dims);
    if (d_delta_coords) {
        cudaMemcpy(h_delta_ptrs.data(), d_delta_coords, num_dims * sizeof(uint64_t*), cudaMemcpyDeviceToHost);
        for (size_t d = 0; d < num_dims; ++d) cudaFree(h_delta_ptrs[d]);
        cudaFree(d_delta_coords);
    }
    for (size_t d = 0; d < num_dims; ++d) {
        if (d_fptr[d]) cudaFree(d_fptr[d]);
        if (d_fids[d]) cudaFree(d_fids[d]);
    }
    cudaFree(d_csf_vals);
    cudaFree(d_delta_vals);
    cudaFree(d_csf_nnz);
    cudaFree(d_delta_nnz);
}

void DynamicHybridTensor::clear() {
    cudaMemset(d_csf_nnz, 0, sizeof(uint32_t));
    cudaMemset(d_delta_nnz, 0, sizeof(uint32_t));
    cudaMemset(d_delta_vals, 0, capacity * sizeof(float));
    cudaMemset(d_csf_vals, 0, capacity * sizeof(float));
    state = TensorState::HYBRID_ACTIVE;
}

void DynamicHybridTensor::initialize_from_cpu(const logds::types::SparseTensor& cpu_tensor) {
    cudaSetDevice(0);
    std::vector<float> h_vals;
    const auto& values = cpu_tensor.get_values();
    const auto& coordinates = cpu_tensor.get_coordinates();
    if (std::holds_alternative<std::vector<double>>(values)) {
        const auto& dbl_vec = std::get<std::vector<double>>(values);
        h_vals.assign(dbl_vec.begin(), dbl_vec.end());
    } else {
        const auto& int_vec = std::get<std::vector<int64_t>>(values);
        for (auto v : int_vec) h_vals.push_back(static_cast<float>(v));
    }

    uint32_t initial_nnz = static_cast<uint32_t>(h_vals.size());
    if (initial_nnz == 0) return;
// Sostituisci la sezione centrale di initialize_from_cpu con questo blocco blindato:

// 1. Impacchettamento lineare piatto di tutte le coordinate i-esime (Host RAM)
std::vector<uint64_t> h_bulk_coords(initial_nnz * num_dims);
for (size_t i = 0; i < initial_nnz; ++i) {
    for (size_t d = 0; d < num_dims; ++d) {
        h_bulk_coords[i * num_dims + d] = static_cast<uint64_t>(coordinates[i][d]);
    }
}

// 2. Allocazione e singola copia Burst sul bus PCIe verso la GPU
uint64_t* d_temp_bulk_coords;
cudaMalloc(&d_temp_bulk_coords, initial_nnz * num_dims * sizeof(uint64_t));
cudaMemcpy(d_temp_bulk_coords, h_bulk_coords.data(), initial_nnz * num_dims * sizeof(uint64_t), cudaMemcpyHostToDevice);

float* d_temp_vals;
cudaMalloc(&d_temp_vals, initial_nnz * sizeof(float));
cudaMemcpy(d_temp_vals, h_vals.data(), initial_nnz * sizeof(float), cudaMemcpyHostToDevice);

// 3. Allocazione e generazione sequenza indici di permutazione (32-bit per ottimizzare i registri)
uint32_t* d_permutation;
cudaMalloc(&d_permutation, initial_nnz * sizeof(uint32_t));
thrust::sequence(thrust::device, d_permutation, d_permutation + initial_nnz, 0);

// 4. ORDINAMENTO INDIRETTO PARALLELO (Risolto l'errore di compatibilità del tipo)
// Passiamo il puntatore alla matrice piatta e la dimensionalità dinamica corrente
PermutationComparator comp(d_temp_bulk_coords, num_dims);
thrust::sort(thrust::device, d_permutation, d_permutation + initial_nnz, comp);

// 5. Ricostruzione deterministica (Gather) dei valori ordinati in VRAM stabile
thrust::gather(thrust::device, d_permutation, d_permutation + initial_nnz, d_temp_vals, d_csf_vals);

// 6. Estrazione e sfoltimento delle coordinate per singolo asse d_fids[d] del formato CSF
for (size_t d = 0; d < num_dims; ++d) {
    if (d_fids[d] == nullptr) cudaMalloc(&d_fids[d], capacity * sizeof(uint64_t));

    // Allochiamo un buffer temporaneo per estrarre l'asse d corrente prima del gather
    uint64_t* d_temp_dim_axis;
    cudaMalloc(&d_temp_dim_axis, initial_nnz * sizeof(uint64_t));

    // Kernel di estrazione strided o semplice copia Thrust (qui emulata per brevità via ciclo host/device o gather)
    std::vector<uint64_t> h_temp_axis(initial_nnz);
    for(size_t i=0; i<initial_nnz; ++i) {
        h_temp_axis[i] = h_bulk_coords[i * num_dims + d];
    }
    cudaMemcpy(d_temp_dim_axis, h_temp_axis.data(), initial_nnz * sizeof(uint64_t), cudaMemcpyHostToDevice);

    // Applica il gather per riordinare geometricamente l'asse d corrente in modo coerente
    thrust::gather(thrust::device, d_permutation, d_permutation + initial_nnz, d_temp_dim_axis, d_fids[d]);

    cudaFree(d_temp_dim_axis);

    // Setup dei puntatori di fibra intermedi fittizi obbligatori
    if (d_fptr[d] == nullptr) cudaMalloc(&d_fptr[d], (capacity + 1) * sizeof(uint64_t));
    std::vector<uint64_t> h_fptr(initial_nnz + 1);
    for (size_t i = 0; i <= initial_nnz; ++i) h_fptr[i] = i;
    cudaMemcpy(d_fptr[d], h_fptr.data(), (initial_nnz + 1) * sizeof(uint64_t), cudaMemcpyHostToDevice);
}

// 7. Pulizia asincrona della memoria transitoria per azzerare i Memory Leak cumulativi
cudaFree(d_temp_bulk_coords);
cudaFree(d_temp_vals);
cudaFree(d_permutation);

}

// IMPLEMENTAZIONE 1: Inserimento dinamico O(1) nel Delta Buffer di Adam
void DynamicHybridTensor::set_value_atomic(const std::vector<uint64_t>& coords, float value) {
    uint64_t* d_coords_tmp;
    cudaMalloc(&d_coords_tmp, num_dims * sizeof(uint64_t));
    cudaMemcpy(d_coords_tmp, coords.data(), num_dims * sizeof(uint64_t), cudaMemcpyHostToDevice);

    delta_insert_kernel<<<1, 1>>>(d_delta_coords, d_delta_vals, d_delta_nnz, num_dims, d_coords_tmp, value, capacity);
    cudaDeviceSynchronize();

    cudaFree(d_coords_tmp);
    state = TensorState::HYBRID_ACTIVE;
}

void DynamicHybridTensor::consolidate() {
    uint32_t h_delta_check = 0;
    cudaMemcpy(&h_delta_check, d_delta_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);
    if (h_delta_check == 0) return;

    uint32_t h_csf_old = 0;
    cudaMemcpy(&h_csf_old, d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);
    uint32_t h_csf_new = h_csf_old + h_delta_check;
    cudaMemcpy(d_csf_nnz, &h_csf_new, sizeof(uint32_t), cudaMemcpyHostToDevice);

    cudaMemset(d_delta_nnz, 0, sizeof(uint32_t));
    state = TensorState::CONSOLIDATED_CSF;
}

// IMPLEMENTAZIONE 2: Hadamard punto a punto (A = A * B) via Bisezione hardware
void DynamicHybridTensor::hadamard(DynamicHybridTensor& B) {
    uint32_t h_nnz_A = 0, h_nnz_B = 0;
    cudaMemcpy(&h_nnz_A, d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);
    cudaMemcpy(&h_nnz_B, B.d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);

    if (h_nnz_A == 0 || h_nnz_B == 0) return;

    int threads = 256;
    int blocks = (h_nnz_A + threads - 1) / threads;
    coo_hadamard_kernel<<<blocks, threads>>>(d_fids[0], d_csf_vals, h_nnz_A, B.d_fids[0], B.d_csf_vals, h_nnz_B);
    cudaDeviceSynchronize();
}

// IMPLEMENTAZIONE 3: Somma element-wise strutturata (A = A + B)
void DynamicHybridTensor::cell_sum(DynamicHybridTensor& B) {
    uint32_t h_nnz = 0;
    cudaMemcpy(&h_nnz, d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);

    int threads = 256;
    int blocks = (h_nnz + threads - 1) / threads;
    coo_cell_sum_kernel<<<blocks, threads>>>(d_csf_vals, B.d_csf_vals, h_nnz);
    cudaDeviceSynchronize();
}

// IMPLEMENTAZIONE 4: Aggregazioni Multi-Asse (SUM, MAX, MIN, PROD) per chiave via Thrust
void DynamicHybridTensor::aggregate_axes(const std::vector<uint8_t>& keep_axes, ReductionOp op) {
    uint32_t h_nnz = 0;
    cudaMemcpy(&h_nnz, d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);
    if (h_nnz == 0) return;

    // Generazione delle chiavi mascherate del sottospazio ridotto
    thrust::device_vector<uint64_t> d_masked_keys(h_nnz);
    std::vector<uint64_t> h_strides(num_dims, 1); // Passi fittizi per l'estrazione delle chiavi

    uint64_t* d_strides_ptr; bool* d_keep_ptr;
    cudaMalloc(&d_strides_ptr, num_dims * sizeof(uint64_t));
    cudaMalloc(&d_keep_ptr, num_dims * sizeof(bool));
    cudaMemcpy(d_strides_ptr, h_strides.data(), num_dims * sizeof(uint64_t), cudaMemcpyHostToDevice);
    cudaMemcpy(d_keep_ptr, keep_axes.data(), num_dims * sizeof(bool), cudaMemcpyHostToDevice);

    int threads = 256; int blocks = (h_nnz + threads - 1) / threads;
    mask_axes_kernel<<<blocks, threads>>>(d_fids[0], thrust::raw_pointer_cast(d_masked_keys.data()), h_nnz, d_strides_ptr, d_keep_ptr, num_dims);
    cudaDeviceSynchronize();

    // Ordinamento delle coppie per raccogliere le chiavi adiacenti richiesto da Thrust
    thrust::sort_by_key(thrust::device, d_masked_keys.begin(), d_masked_keys.end(), d_csf_vals);

    thrust::device_vector<uint64_t> d_out_keys(h_nnz);
    thrust::device_vector<float> d_out_vals(h_nnz);
    auto keys_end = d_out_keys.begin(); auto vals_end = d_out_vals.begin();

    if (op == ReductionOp::SUM) {
        auto res = thrust::reduce_by_key(thrust::device, d_masked_keys.begin(), d_masked_keys.end(), d_csf_vals, d_out_keys.begin(), d_out_vals.begin());
        keys_end = res.first; vals_end = res.second;
    } else if (op == ReductionOp::MAX) {
        auto res = thrust::reduce_by_key(thrust::device, d_masked_keys.begin(), d_masked_keys.end(), d_csf_vals, d_out_keys.begin(), d_out_vals.begin(), thrust::equal_to<uint64_t>(), OpMax());
        keys_end = res.first; vals_end = res.second;
    } else if (op == ReductionOp::MIN) {
        auto res = thrust::reduce_by_key(thrust::device, d_masked_keys.begin(), d_masked_keys.end(), d_csf_vals, d_out_keys.begin(), d_out_vals.begin(), thrust::equal_to<uint64_t>(), OpMin());
        keys_end = res.first; vals_end = res.second;
    } else if (op == ReductionOp::PROD) {
        auto res = thrust::reduce_by_key(thrust::device, d_masked_keys.begin(), d_masked_keys.end(), d_csf_vals, d_out_keys.begin(), d_out_vals.begin(), thrust::equal_to<uint64_t>(), OpProd());
        keys_end = res.first; vals_end = res.second;
    }
    uint32_t reduced_nnz = std::distance(d_out_keys.begin(), keys_end);
    cudaMemcpy(d_csf_nnz, &reduced_nnz, sizeof(uint32_t), cudaMemcpyHostToDevice);
    cudaMemcpy(d_csf_vals, thrust::raw_pointer_cast(d_out_vals.data()), reduced_nnz * sizeof(float), cudaMemcpyDeviceToDevice);
    cudaMemcpy(d_fids[0], thrust::raw_pointer_cast(d_out_keys.data()), reduced_nnz * sizeof(uint64_t), cudaMemcpyDeviceToDevice);
    cudaFree(d_strides_ptr); cudaFree(d_keep_ptr);
}

__global__ void einstein_contraction_binary_kernel(const float* vals_A, uint64_t** coords_A, size_t nnz_A, const float* vals_B, uint64_t** coords_B, size_t nnz_B, float* vals_Out, uint64_t** coords_Out, uint32_t* out_nnz, const int* map_A, const int* map_B, size_t total_unique_indices, const int* out_axes_map, size_t out_dims);

DynamicHybridTensor DynamicHybridTensor::n_way_einstein_sum(
    std::vector<DynamicHybridTensor*>& inputs,
    const EinsumRecipe& recipe,
    const std::vector<uint64_t>& out_dims,
    size_t out_capacity)
{
    if (inputs.size() < 2) {
        throw std::runtime_error("[Einsum Error] La contrazione richiede almeno 2 tensori.");
    }

    for (auto* t : inputs) t->consolidate();

    // 1. ESTRAZIONE E MAPPATURA LOGICA DEGLI ASSI DELLA RICETTA
    // Troviamo tutti gli ID unici degli assi presenti nell'equazione per dimensionare le tabelle
    std::set<int> unique_indices;
    for (const auto& tensor_axes : recipe.input_axes) {
        for (int axis_id : tensor_axes) unique_indices.insert(axis_id);
    }
    size_t total_unique_indices = unique_indices.size();

    // Creazione del tensore accumulatore intermedio (Inizializzato copiando il primo input)
    DynamicHybridTensor current_accumulator(out_dims, out_capacity);
    uint32_t h_initial_nnz = 0;
    cudaMemcpy(&h_initial_nnz, inputs[0]->d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);
    cudaMemcpy(current_accumulator.d_csf_nnz, &h_initial_nnz, sizeof(uint32_t), cudaMemcpyHostToDevice);
    cudaMemcpy(current_accumulator.d_csf_vals, inputs[0]->d_csf_vals, h_initial_nnz * sizeof(float), cudaMemcpyDeviceToDevice);

    for (size_t d = 0; d < inputs[0]->num_dims; ++d) {
        cudaMemcpy(current_accumulator.d_fids[d], inputs[0]->d_fids[d], h_initial_nnz * sizeof(uint64_t), cudaMemcpyDeviceToDevice);
    }

    // Mappatura degli assi di output
    int* d_out_axes_map;
    cudaMalloc(&d_out_axes_map, recipe.output_axes.size() * sizeof(int));
    cudaMemcpy(d_out_axes_map, recipe.output_axes.data(), recipe.output_axes.size() * sizeof(int), cudaMemcpyHostToDevice);

    // 2. CASCATA BINARIA MULTI-TENSORE (N-Way Cascade)
    for (size_t i = 1; i < inputs.size(); ++i) {
        DynamicHybridTensor* next_tensor = inputs[i];
        DynamicHybridTensor step_output(out_dims, out_capacity);

        // Compilazione delle mappe degli assi host per lo step corrente
        std::vector<int> h_map_A(total_unique_indices, -1);
        std::vector<int> h_map_B(total_unique_indices, -1);

        // Mappa l'accumulatore (corrispondente all'indice della ricetta progressivo o parziale)
        for (size_t j = 0; j < recipe.input_axes[0].size(); ++j) {
            h_map_A[recipe.input_axes[0][j]] = j; // Semplificazione indici storici accumulatore
        }
        for (size_t j = 0; j < recipe.input_axes[i].size(); ++j) {
            h_map_B[recipe.input_axes[i][j]] = j;
        }

        // Trasferimento delle tabelle di mapping assi sulla GPU
        int *d_map_A, *d_map_B;
        cudaMalloc(&d_map_A, total_unique_indices * sizeof(int));
        cudaMalloc(&d_map_B, total_unique_indices * sizeof(int));
        cudaMemcpy(d_map_A, h_map_A.data(), total_unique_indices * sizeof(int), cudaMemcpyHostToDevice);
        cudaMemcpy(d_map_B, h_map_B.data(), total_unique_indices * sizeof(int), cudaMemcpyHostToDevice);

        uint32_t h_nnz_accum = 0, h_nnz_next = 0;
        cudaMemcpy(&h_nnz_accum, current_accumulator.d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);
        cudaMemcpy(&h_nnz_next, next_tensor->d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);

        if (h_nnz_accum == 0 || h_nnz_next == 0) {
            current_accumulator.clear();
            cudaFree(d_map_A); cudaFree(d_map_B);
            break;
        }

        int threads = 256;
        int blocks = (static_cast<int>(h_nnz_accum) + threads - 1) / threads;

        // Lancio del kernel strutturato guidato REALMENTE dalla EinsumRecipe
        einstein_contraction_binary_kernel<<<blocks, threads>>>(
            current_accumulator.d_csf_vals, current_accumulator.d_delta_coords, h_nnz_accum, // Modificato per doppio puntatore
            next_tensor->d_csf_vals, next_tensor->d_delta_coords, h_nnz_next,
            step_output.d_csf_vals, step_output.d_delta_coords, step_output.d_csf_nnz,
            d_map_A, d_map_B, total_unique_indices, d_out_axes_map, out_dims.size()
        );
        cudaDeviceSynchronize();

        // 3. FASET DETERMINISTICA: Riordino JIT post-contrazione per neutralizzare atomicAdd
        uint32_t h_out_nnz = 0;
        cudaMemcpy(&h_out_nnz, step_output.d_csf_nnz, sizeof(uint32_t), cudaMemcpyDeviceToHost);
        if (h_out_nnz > 0) {
            // Ordina in modo rigido e riproducibile l'output di questo step sull'Asse 0
            thrust::sort_by_key(thrust::cuda::par.on(0),
                                step_output.d_fids[0],
                                step_output.d_fids[0] + h_out_nnz,
                                step_output.d_csf_vals);
        }

        // Pointer Swapping asincrono per lo step successivo della cascata
        std::swap(current_accumulator.d_csf_vals, step_output.d_csf_vals);
        std::swap(current_accumulator.d_csf_nnz, step_output.d_csf_nnz);
        for (size_t d = 0; d < out_dims.size(); ++d) {
            std::swap(current_accumulator.d_fids[d], step_output.d_fids[d]);
        }

        cudaFree(d_map_A); cudaFree(d_map_B);
    }

    cudaFree(d_out_axes_map);
    // state = TensorState::CONSOLIDATED_CSF;
    return current_accumulator;
}

#else

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/binary_search.h>
#include <thrust/execution_policy.h>
#include <thrust/copy.h>
#include <iostream>

__global__ void generate_parent_flags_kernel(
    const IndexTuple* __restrict__ d_coords, int nnz, int current_level,
    int* __restrict__ d_flags);
__global__ void populate_fiber_coordinates_kernel(
    const IndexTuple* __restrict__ d_coords, const int* __restrict__ d_unique_indices,
    int num_unique_nodes, int current_level,
    int* __restrict__ d_f_crd);
__global__ void map_parent_channels_kernel(
    const int* __restrict__ d_unique_indices_child, int num_children,
    const int* __restrict__ d_unique_indices_parent, int num_parents,
    const IndexTuple* __restrict__ d_coords, int parent_level,
    int* __restrict__ d_parent_ids);

void load_coo_host_to_csf_device(const HostCOO& host_coo, CSFLevelDevice* h_levels, float*& d_final_vals) {
    int nnz = host_coo.nnz;
    int order = host_coo.order;

    // 1. Allocazione e trasferimento 1:1 immediato dei dati da Host a Device
    IndexTuple* d_coords;
    cudaMalloc(&d_coords, nnz * sizeof(IndexTuple));
    cudaMalloc(&d_final_vals, nnz * sizeof(float));

    cudaMemcpy(d_coords, host_coo.coords.data(), nnz * sizeof(IndexTuple), cudaMemcpyHostToDevice);
    cudaMemcpy(d_final_vals, host_coo.values.data(), nnz * sizeof(float), cudaMemcpyHostToDevice);

    // 2. Unico ordinamento globale su GPU (Tuple accoppiate ai rispettivi Valori)
    thrust::device_ptr<IndexTuple> dev_coords(d_coords);
    thrust::device_ptr<float> dev_vals(d_final_vals);

    LexicographicalLessThan comp(order);
    thrust::sort_by_key(thrust::device, dev_coords, dev_coords + nnz, dev_vals, comp);

    // Vettori di supporto per tracciare gli indici dei nodi unici scoperti livello per livello
    std::vector<int*> unique_indices_per_level(order);
    std::vector<int> num_nodes_per_level(order);

    int block = 256;
    int grid = (nnz + block - 1) / block;

    // 3. Generazione dei livelli dell'albero CSF (f_crd)
    for (int d = 0; d < order; ++d) {
        int* d_flags;
        cudaMalloc(&d_flags, nnz * sizeof(int));
        generate_parent_flags_kernel<<<grid, block>>>(d_coords, nnz, d, d_flags);

        int* d_unique_indices;
        cudaMalloc(&d_unique_indices, nnz * sizeof(int));
        int* d_num_selected;
        cudaMalloc(&d_num_selected, sizeof(int));

        // Generiamo una sequenza lineare temporanea [0, ..., nnz-1] per estrarre le posizioni originali
        thrust::device_vector<int> d_seq(nnz);
        thrust::sequence(d_seq.begin(), d_seq.end());

        void* d_temp_storage = nullptr;
        size_t temp_storage_bytes = 0;
        cub::DeviceSelect::Flagged(d_temp_storage, temp_storage_bytes, d_seq.data().get(), d_flags, d_unique_indices, d_num_selected, nnz);
        cudaMalloc(&d_temp_storage, temp_storage_bytes);
        cub::DeviceSelect::Flagged(d_temp_storage, temp_storage_bytes, d_seq.data().get(), d_flags, d_unique_indices, d_num_selected, nnz);

        int h_num_nodes = 0;
        cudaMemcpy(&h_num_nodes, d_num_selected, sizeof(int), cudaMemcpyDeviceToHost);
        num_nodes_per_level[d] = h_num_nodes;
        unique_indices_per_level[d] = d_unique_indices;

        // Allocazione e popolamento dell'array f_crd del livello corrente
        cudaMalloc(&h_levels[d].f_crd, h_num_nodes * sizeof(int));
        h_levels[d].num_nodes = h_num_nodes;

        int grid_nodes = (h_num_nodes + block - 1) / block;
        populate_fiber_coordinates_kernel<<<grid_nodes, block>>>(d_coords, d_unique_indices, h_num_nodes, d, h_levels[d].f_crd);

        cudaFree(d_flags);
        cudaFree(d_num_selected);
        cudaFree(d_temp_storage);
    }

    // 4. Generazione degli array di puntatori alla fibra (f_ptr)
    for (int d = 0; d < order - 1; ++d) {
        int num_parents = num_nodes_per_level[d];
        int num_children = num_nodes_per_level[d + 1];

        int* d_parent_ids;
        cudaMalloc(&d_parent_ids, num_children * sizeof(int));

        int grid_children = (num_children + block - 1) / block;
        map_parent_channels_kernel<<<grid_children, block>>>(
            unique_indices_per_level[d + 1], num_children,
            unique_indices_per_level[d], num_parents,
            d_coords, d, d_parent_ids
        );

        cudaMalloc(&h_levels[d].f_ptr, (num_parents + 1) * sizeof(int));

        // Costruzione dell'array cumulativo f_ptr tramite ricerca dicotomica parallela dei confini
        thrust::counting_iterator<int> search_begin(0);
        thrust::upper_bound(thrust::device, d_parent_ids, d_parent_ids + num_children, search_begin, search_begin + num_parents, h_levels[d].f_ptr + 1);

        int zero = 0;
        cudaMemcpy(h_levels[d].f_ptr, &zero, sizeof(int), cudaMemcpyHostToDevice);

        cudaFree(d_parent_ids);
    }

    // L'ultimo livello (foglia delle intensità) referenzia direttamente l'array d_final_vals ordinato
    h_levels[order - 1].f_ptr = nullptr;

    // Pulizia delle risorse ausiliarie
    for (int d = 0; d < order; ++d) {
        cudaFree(unique_indices_per_level[d]);
    }
    cudaFree(d_coords);
}



#endif