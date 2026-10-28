//
// Created by gyankos on 24/10/26.
//

#ifndef PARAMONEUR2_SPARSETENSOR_H
#define PARAMONEUR2_SPARSETENSOR_H

#include <cstdint>
#include <variant>
#include <vector>
#include <cmath>
#include <filesystem>
#include <functional>
#include <string>

// Tipo consolidato per la decodifica testuale delle coordinate: (dim_id, coord_index) -> stringa
using LabelProvider = std::function<std::string(int64_t, int64_t)>;


namespace logds::types {

    enum class TensorDataType : uint8_t {
        INT64 = 1,
        DOUBLE = 2
    };


class SparseTensor {
public:
    // Tipo per i valori alloggiati nella variante per astrazione uniforme
    using TensorValueVariant = std::variant<int64_t, double>;

    // 1. COSTRUTTORE TRIVIALE (Richiesto per la deserializzazione da disco / speculazione fstream)
    SparseTensor();

    // 2. COSTRUTTORE PARAMETRIZZATO CON INIZIALIZZAZIONE DI DEFAULT A ZERO

    SparseTensor(const std::vector<size_t>& tensor_shape, double default_value = 0.0);
    SparseTensor(const std::vector<size_t>& tensor_shape, int64_t default_value = 0);
    SparseTensor(TensorDataType type, const std::vector<size_t>& tensor_shape);
    SparseTensor(const std::filesystem::path& path, double default_value = 0.0);

    // INTERFACCIA PUBBLICA DI SOLA LETTURA (GETTERS)
    [[nodiscard]] TensorDataType get_data_type() const { return data_type; }
    [[nodiscard]] size_t get_rank() const { return rank; }
    [[nodiscard]] const std::vector<size_t>& get_shape() const { return shape; }
    [[nodiscard]] const TensorValueVariant& get_default_value() const { return default_value; }
    [[nodiscard]] const std::vector<std::vector<size_t>>& get_coordinates() const { return coordinates; }

    // Ritorna la variante contenente il vettore corretto di elementi non nulli
    [[nodiscard]] const std::variant<std::vector<int64_t>, std::vector<double>>& get_values() const { return values; }

    // 5. SETTER / AGGIORNAMENTO DI UN ELEMENTO COO CON REGOLE DI FILTRO E GUARDRAIL
    // Ritorna true se l'operazione ha effetto (inserimento o aggiornamento riuscito), false se ignorata
    bool set_value(const std::vector<size_t>& coord, const TensorValueVariant& val);

    // Estrazione pulita di un elemento (Ritorna il valore se presente nelle coordinate, altrimenti il default)
    TensorValueVariant get_value(const std::vector<size_t>& coord) const;

    // Converte questo tensore coo in una matrice 2-dimensionale efficiente per cuda in rappresentazione CSR
    void convert_nested_coo_to_csr_cpu(
        std::vector<int>& csrRowPtr,                 // Output puntatori di riga (dim: num_rows + 1)
        std::vector<int>& csrColInd                  // Output indici di colonna (dim: nnz)
    );



    // 1) Funzione che salva lo stack di heatmaps su file senza effettuare il rendering a schermo
void save_tensor_heatmaps_to_file(
    const std::string& output_filepath,
    const LabelProvider& label_provider);

    // 3) Funzione per plottare la tabella strutturata a terminale via p-ranav/tabulate
void terminal_plot_mytensor(
    double threshold,
    const LabelProvider& label_provider);


    // Helper per convertire i dati sparsi di Mytensor relativi a una specifica slice 2D in una matrice densa per Matplot++
    std::vector<std::vector<double>> extract_2d_slice(
        const std::vector<size_t>& current_dims,
        size_t dim_h, size_t dim_w,
        size_t size_h, size_t size_w);

protected:
    // 3. CAMPI PROTETTI ACCESSIBILI SOLO TRAMITE GETTER/SETTER CONTROLLATI
    TensorDataType data_type;
    size_t rank;
    std::vector<size_t> shape;
    TensorValueVariant default_value;

    // Matrici del layout COO sparse
    std::vector<std::vector<size_t>> coordinates;
    std::variant<std::vector<int64_t>, std::vector<double>> values;
};

}

#endif //PARAMONEUR2_SPARSETENSOR_H
