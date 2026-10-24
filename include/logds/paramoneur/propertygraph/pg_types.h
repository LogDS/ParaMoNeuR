//
// Created by gyankos on 23/10/26.
//

#ifndef PARAMONEUR2_PG_TYPES_H
#define PARAMONEUR2_PG_TYPES_H

#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <cstdint>
namespace logds::pg {


    constexpr  uint64_t DISK_NULL = 0xFFFFFFFFFFFFFFFF;

    enum class PropertyType : uint8_t {
        INT = 1,
        DOUBLE = 2,
        STRING = 3,
        BOOL = 4,
        SPARSE_TENSOR = 5
    };

    enum class TensorDataType : uint8_t {
        INT64 = 1,
        DOUBLE = 2
    };


class SparseTensor {
public:
    // Tipo per i valori alloggiati nella variante per astrazione uniforme
    using TensorValueVariant = std::variant<int64_t, double>;

    // 1. COSTRUTTORE TRIVIALE (Richiesto per la deserializzazione da disco / speculazione fstream)
    SparseTensor()
        : data_type(TensorDataType::DOUBLE), rank(0), default_value(0.0) {}

    // 2. COSTRUTTORE PARAMETRIZZATO CON INIZIALIZZAZIONE DI DEFAULT A ZERO

    SparseTensor(const std::vector<size_t>& tensor_shape, double default_value = 0.0)
    : data_type(TensorDataType::DOUBLE), shape(tensor_shape), rank(tensor_shape.size())
    {
        this->default_value = default_value;
    }

    SparseTensor(const std::vector<size_t>& tensor_shape, int64_t default_value = 0)
: data_type(TensorDataType::INT64), shape(tensor_shape), rank(tensor_shape.size())
    {
        this->default_value = default_value;
    }

    SparseTensor(TensorDataType type, const std::vector<size_t>& tensor_shape)
        : data_type(type), shape(tensor_shape), rank(tensor_shape.size())
    {
        if (data_type == TensorDataType::INT64) {
            default_value = int64_t(0);
        } else {
            default_value = double(0.0);
        }
    }

    // INTERFACCIA PUBBLICA DI SOLA LETTURA (GETTERS)
    TensorDataType get_data_type() const { return data_type; }
    size_t get_rank() const { return rank; }
    const std::vector<size_t>& get_shape() const { return shape; }
    const TensorValueVariant& get_default_value() const { return default_value; }
    const std::vector<std::vector<size_t>>& get_coordinates() const { return coordinates; }

    // Ritorna la variante contenente il vettore corretto di elementi non nulli
    const std::variant<std::vector<int64_t>, std::vector<double>>& get_values() const { return values; }

    // 5. SETTER / AGGIORNAMENTO DI UN ELEMENTO COO CON REGOLE DI FILTRO E GUARDRAIL
    // Ritorna true se l'operazione ha effetto (inserimento o aggiornamento riuscito), false se ignorata
    bool set_value(const std::vector<size_t>& coord, const TensorValueVariant& val) {
        // Guardrail A: Convalida del tipo di dato in ingresso rispetto al tipo nativo impostato
        if ((data_type == TensorDataType::INT64 && !std::holds_alternative<int64_t>(val)) ||
            (data_type == TensorDataType::DOUBLE && !std::holds_alternative<double>(val))) {
            return false; // Conflitto di tipo rilevato, l'operazione fallisce silenziosamente
        }

        // Guardrail B: Verifica se la coordinata è fuori dalle dimensioni (Out of Bounds)
        if (coord.size() != rank) return false;
        for (size_t i = 0; i < rank; ++i) {
            if (coord[i] >= shape[i]) {
                return false; // Ignorato silenziosamente senza lanciare eccezioni
            }
        }

        // Guardrail C: Verifica se il valore è inferiore alla soglia di tolleranza decimale
        if (data_type == TensorDataType::DOUBLE) {
            double v_double = std::get<double>(val);
            if (std::abs(v_double) <= std::get<double>(default_value)) {
                return false; // Non efficace per preservare la sparsità strutturale
            }
        } else {
            int64_t v_int = std::get<int64_t>(val);
            if (std::abs(static_cast<double>(v_int)) <= std::get<double>(default_value)) {
                return false; // Non efficace per interi vicini o uguali a zero
            }
        }

        // Guardrail D: Gestione sovrascrittura se la coordinata era già presente (Evita duplicazioni COO)
        for (size_t i = 0; i < coordinates.size(); ++i) {
            if (coordinates[i] == coord) {
                if (data_type == TensorDataType::INT64) {
                    if (!std::holds_alternative<std::vector<int64_t>>(values)) {
                        values = std::vector<int64_t>(coordinates.size(), 0);
                    }
                    std::get<std::vector<int64_t>>(values)[i] = std::get<int64_t>(val);
                } else {
                    if (!std::holds_alternative<std::vector<double>>(values)) {
                        values = std::vector<double>(coordinates.size(), 0.0);
                    }
                    std::get<std::vector<double>>(values)[i] = std::get<double>(val);
                }
                return true; // Aggiornamento eseguito sul posto
            }
        }

        // Se non esisteva, eseguiamo l'append in coda (Nuovo elemento COO valido)
        coordinates.push_back(coord);
        if (data_type == TensorDataType::INT64) {
            if (!std::holds_alternative<std::vector<int64_t>>(values)) {
                values = std::vector<int64_t>();
            }
            std::get<std::vector<int64_t>>(values).push_back(std::get<int64_t>(val));
        } else {
            if (!std::holds_alternative<std::vector<double>>(values)) {
                values = std::vector<double>();
            }
            std::get<std::vector<double>>(values).push_back(std::get<double>(val));
        }
        return true;
    }

    // Estrazione pulita di un elemento (Ritorna il valore se presente nelle coordinate, altrimenti il default)
    TensorValueVariant get_value(const std::vector<size_t>& coord) const {
        if (coord.size() != rank) return default_value;

        for (size_t i = 0; i < coordinates.size(); ++i) {
            if (coordinates[i] == coord) {
                if (data_type == TensorDataType::INT64) {
                    return std::get<std::vector<int64_t>>(values)[i];
                } else {
                    return std::get<std::vector<double>>(values)[i];
                }
            }
        }
        return default_value;
    }

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

    struct PropertyValue {
        PropertyType type;
        std::variant<int64_t, double, std::string, bool, SparseTensor> value;
    };

    struct NodeData {
        size_t id;
        std::vector<std::string> labels;
        std::unordered_map<std::string, PropertyValue> properties;
    };

    struct EdgeData {
        size_t id;
        size_t source_id;
        size_t target_id;
        std::vector<std::string> labels;
        std::unordered_map<std::string, PropertyValue> properties;
    };

    struct NodeRecord {
        uint64_t payload_offset = DISK_NULL;
        uint64_t payload_size = 0;
        uint64_t first_outgoing = DISK_NULL;
        uint64_t first_ingoing = DISK_NULL;
        bool exists = false;
    };

    struct EdgeRecord {
        size_t source_id = 0;
        size_t target_id = 0;
        uint64_t payload_offset = DISK_NULL;
        uint64_t payload_size = 0;
        uint64_t next_outgoing = DISK_NULL;
        uint64_t next_ingoing = DISK_NULL;
        bool exists = false;
    };

    struct EdgeTuple {
        size_t source;
        size_t target;
        size_t edge_id;
    };
// #endif // PROPERTY_GRAPH_TYPES_H
}

#endif //PARAMONEUR2_PG_TYPES_H
