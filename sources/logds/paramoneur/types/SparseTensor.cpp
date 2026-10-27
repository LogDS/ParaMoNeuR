//
// Created by gyankos on 24/10/26.
//

#include "logds/paramoneur/types/SparseTensor.h"

namespace logds::types {
    SparseTensor::SparseTensor(): data_type(TensorDataType::DOUBLE), rank(0), default_value(0.0) {}

    SparseTensor::SparseTensor(const std::vector<size_t> &tensor_shape, double default_value): data_type(TensorDataType::DOUBLE), shape(tensor_shape), rank(tensor_shape.size()) {
        this->default_value = default_value;
    }

    SparseTensor::SparseTensor(const std::vector<size_t> &tensor_shape, int64_t default_value): data_type(TensorDataType::INT64), shape(tensor_shape), rank(tensor_shape.size()) {
        this->default_value = default_value;
    }

    SparseTensor::SparseTensor(TensorDataType type, const std::vector<size_t> &tensor_shape): data_type(type), shape(tensor_shape), rank(tensor_shape.size()) {
        if (data_type == TensorDataType::INT64) {
            default_value = int64_t(0);
        } else {
            default_value = double(0.0);
        }
    }

    bool SparseTensor::set_value(const std::vector<size_t> &coord, const TensorValueVariant &val) {
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

    SparseTensor::TensorValueVariant SparseTensor::get_value(const std::vector<size_t> &coord) const {
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
}
