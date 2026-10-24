//
// Created by gyankos on 23/10/26.
//

#ifndef PARAMONEUR2_PG_STORE_H
#define PARAMONEUR2_PG_STORE_H

#include "pg_types.h"
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <memory>
#include <typeindex>
#include <stdexcept>
#include <field_reflection.hpp>

namespace logds::pg {

    class PropertyGraphTypeRegistry {
    private:
        std::unordered_map<std::string, std::type_index> string_to_index;
        std::unordered_map<std::type_index, std::string> index_to_string;

        PropertyGraphTypeRegistry() = default;

    public:
        static PropertyGraphTypeRegistry& instance() {
            static PropertyGraphTypeRegistry reg;
            return reg;
        }

        template <typename T>
        void register_type() {
            std::string name{ field_reflection::type_name<T> };
            std::type_index idx = std::type_index(typeid(T));

            // CORREZIONE: usiamo insert_or_assign o emplace per bypassare il costruttore di default
            string_to_index.insert_or_assign(name, idx);
            index_to_string.insert_or_assign(idx, name);
        }

        bool get_type_index(const std::string& name, std::type_index& out_idx) const {
            auto it = string_to_index.find(name);
            if (it != string_to_index.end()) {
                out_idx = it->second; // Assegnamento copia lecito
                return true;
            }
            return false;
        }

        std::string get_type_name(std::type_index idx) const {
            auto it = index_to_string.find(idx);
            if (it != index_to_string.end()) {
                return it->second;
            }
            return "Unknown_Type";
        }
    };


class PropertyGraphStore;
class Transaction;

// Proxy per intercettare letture e scritture sulle proprieta tramite operator[]
class PropertyProxy {
private:
    std::unordered_map<std::string, PropertyValue>& properties_ref;
    std::string key;
    bool& dirty_flag_ref;

public:
    PropertyProxy(std::unordered_map<std::string, PropertyValue>& props, const std::string& k, bool& dirty);
    operator PropertyValue() const;
    PropertyProxy& operator=(const PropertyValue& val);
    PropertyProxy& operator=(const types::SparseTensor& val) {
        PropertyValue prop{PropertyType::SPARSE_TENSOR, val};
        return *this=prop;
    }
    PropertyProxy& operator=(double val) {
        PropertyValue prop{ PropertyType::DOUBLE, double(val) };
        return *this=prop;
    }
    PropertyProxy& operator=(char* val) {
        logds::pg::PropertyValue prop{ logds::pg::PropertyType::STRING, std::string(val) };
        return *this=prop;
    }
    PropertyProxy& operator=(bool val) {
        logds::pg::PropertyValue prop{ logds::pg::PropertyType::BOOL, val };
        return *this=prop;
    }
    PropertyProxy& operator=(int val) {
        logds::pg::PropertyValue prop{ logds::pg::PropertyType::INT, int64_t(val) };
        return *this=prop;
    }
    PropertyProxy& operator=(int64_t val) {
        logds::pg::PropertyValue prop{ logds::pg::PropertyType::INT, int64_t(val) };
        return *this=prop;
    }
    PropertyProxy& operator=(const std::string& val) {
        logds::pg::PropertyValue prop{logds::pg::PropertyType::STRING, val };
        return *this=prop;
    }
};

// Proxy per l'accesso controllato alle proprieta del nodo
class NodeProxy {
private:
    NodeData& data;
    bool& dirty_flag;

public:
    NodeProxy(NodeData& d, bool& df);
    PropertyProxy operator[](const std::string& key);
    std::vector<std::string>& labels();
    size_t id() const;
};

// Proxy per l'accesso controllato alle proprieta dell'arco
class EdgeProxy {
private:
    EdgeData& data;
    bool& dirty_flag;

public:
    EdgeProxy(EdgeData& d, bool& df);
    PropertyProxy operator[](const std::string& key);
    std::vector<std::string>& labels();
    size_t id() const;
};

// Classe transazionale per operare localmente in isolamento e validare l'esistenza
class Transaction {
friend class PropertyGraphStore;
private:
    PropertyGraphStore& store;
    std::unordered_map<size_t, NodeData> cached_nodes;
    std::unordered_map<size_t, EdgeData> cached_edges;
    std::unordered_map<size_t, bool> dirty_nodes;
    std::unordered_map<size_t, bool> dirty_edges;

    // Contatori locali transazionali per gestire la sequenza incrementale
    size_t next_node_id_local;
    size_t next_edge_id_local;



public:
    Transaction(PropertyGraphStore& s);

    // Metodi di creazione: non accettano ID, ma RESTITUISCONO l'ID autogenerato
    size_t create_node();
    size_t create_edge(size_t source_id, size_t target_id);
    bool edge_exists_context(size_t node_id);
    bool node_exists_context(size_t node_id);

    NodeProxy operator[](size_t node_id);
    EdgeProxy get_edge(size_t edge_id);

    // 1. SET STRUTTURA: Salva i dati e scrive il nome letterale della classe (Nessun Hash)
    template <typename T>
    void set_node_struct(size_t node_id, const T& obj) {
        static_assert(std::is_aggregate_v<T>, "La riflessione richiede una struct aggregata (POD).");

        // Registra automaticamente il tipo nel registro a runtime al primo inserimento
        PropertyGraphTypeRegistry::instance().register_type<T>();

        // Salviamo il nome stringa completo e immutabile determinato dal compilatore
        std::string t_name{ field_reflection::type_name<T> };
        (*this)[node_id]["__type_name"] = PropertyValue{ PropertyType::STRING, t_name };

        // Serializzazione riflessa dei singoli campi
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            auto serialize_field = [&](auto index_constant) {
                constexpr std::size_t I = decltype(index_constant)::value;
                std::string key{ field_reflection::field_name<T, I> };
                const auto& field_val = field_reflection::get_field<I>(obj);
                using FieldType = std::decay_t<decltype(field_val)>;

                if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, int64_t>) {
                    (*this)[node_id][key] = PropertyValue{ PropertyType::INT, int64_t(field_val) };
                } else if constexpr (std::is_same_v<FieldType, double> || std::is_same_v<FieldType, float>) {
                    (*this)[node_id][key] = PropertyValue{ PropertyType::DOUBLE, double(field_val) };
                } else if constexpr (std::is_same_v<FieldType, std::string>) {
                    (*this)[node_id][key] = PropertyValue{ PropertyType::STRING, field_val };
                } else if constexpr (std::is_same_v<FieldType, bool>) {
                    (*this)[node_id][key] = PropertyValue{ PropertyType::BOOL, field_val };
                } else if constexpr (std::is_same_v<FieldType, types::SparseTensor>) {
                    (*this)[node_id][key] = PropertyValue{ PropertyType::SPARSE_TENSOR, field_val };
                }
            };
            (serialize_field(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<field_reflection::field_count<T>>());
    }

    // 2. GET STRUTTURA: Confronto stringa-stringa biunivoco (Garanzia Matematica anti-collisione)
    template <typename T>
    T get_node_struct(size_t node_id) {
        static_assert(std::is_aggregate_v<T>, "La riflessione richiede una struct aggregata (POD).");

        std::string expected_name{ field_reflection::type_name<T> };
        std::string stored_name = get_node_type_string(node_id);

        if (stored_name.empty()) {
            throw std::runtime_error("Type Error: Node " + std::to_string(node_id) + " has no C++ struct associated.");
        }

        // Il confronto tra stringhe garantisce l'assoluta univocita dell'operazione
        if (stored_name != expected_name) {
            throw std::runtime_error("Type Mismatch: Cannot deserialize Node " + std::to_string(node_id) +
                                     ". Target expected type '" + expected_name +
                                     "' but database holds '" + stored_name + "'.");
        }

        T obj{};
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            auto deserialize_field = [&](auto index_constant) {
                constexpr std::size_t I = decltype(index_constant)::value;
                std::string key{ field_reflection::field_name<T, I> };
                auto& field_val = field_reflection::get_field<I>(obj);
                using FieldType = std::decay_t<decltype(field_val)>;

                PropertyValue p_val = (*this)[node_id][key];

                if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, int64_t>) {
                    field_val = static_cast<FieldType>(std::get<int64_t>(p_val.value));
                } else if constexpr (std::is_same_v<FieldType, double> || std::is_same_v<FieldType, float>) {
                    field_val = static_cast<FieldType>(std::get<double>(p_val.value));
                } else if constexpr (std::is_same_v<FieldType, std::string>) {
                    field_val = std::get<std::string>(p_val.value);
                } else if constexpr (std::is_same_v<FieldType, bool>) {
                    field_val = std::get<bool>(p_val.value);
                } else if constexpr (std::is_same_v<FieldType, types::SparseTensor>) {
                    field_val = std::get<types::SparseTensor>(p_val.value);
                }
            };
            (deserialize_field(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<field_reflection::field_count<T>>());

        return obj;
    }

    // 3. QUERY DEL NOME TIPO: Ritorna la stringa letterale memorizzata
    std::string get_node_type_string(size_t node_id) {
        try {
            PropertyValue p_val = (*this)[node_id]["__type_name"];
            return std::get<std::string>(p_val.value);
        } catch (...) {
            return "";
        }
    }

    // 4. QUERY DI TIPO STANDARD: Ritorna il std::type_index nativo accoppiato al record di disco
    // Ritorna std::optional poiche std::type_index non puo essere nullo o default-constructed
    std::optional<std::type_index> get_node_type_index(size_t node_id) {
        std::string type_str = get_node_type_string(node_id);
        if (type_str.empty()) {
            return std::nullopt; // Il nodo esiste ma non e associato a nessuna struct riflessa
        }

        // Struttura temporanea inizializzata con un tipo di fallback reale valido per la firma
        std::type_index idx = std::type_index(typeid(void));
        if (PropertyGraphTypeRegistry::instance().get_type_index(type_str, idx)) {
            return idx;
        }
        return std::nullopt; // Tipo non ancora registrato nella sessione corrente
    }

    // 1. VERIFICA DI COMPATIBILITÀ: Ritorna true se il tipo T è compatibile con quello sul disco
    template <typename T>
    bool is_node_type_compatible(size_t node_id) {
        // Vincolo statico: T deve essere una struct valida per il nostro sistema di riflessione
        if constexpr (!std::is_aggregate_v<T>) {
            return false;
        }

        std::string stored_name = get_node_type_string(node_id);
        std::string requested_name{ field_reflection::type_name<T> };

        // In questo modello la compatibilità è l'uguaglianza biunivoca del nome della classe.
        // Se in futuro introducessi l'ereditarietà strutturale, potresti espandere questa logica.
        return (stored_name == requested_name);
    }

    // 2. RECUPERO SICURO: Popola l'optional solo se il tipo è compatibile, senza lanciare eccezioni
    template <typename T>
    std::optional<T> try_get_node_struct(size_t node_id) {
        // Se il tipo non è compatibile (strutturalmente o a runtime), ritorna un optional vuoto
        if (!is_node_type_compatible<T>(node_id)) {
            return std::nullopt;
        }

        // Se è compatibile, procediamo al caricamento sicuro dei dati
        T obj{};
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            auto deserialize_field = [&](auto index_constant) {
                constexpr std::size_t I = decltype(index_constant)::value;
                std::string key{ field_reflection::field_name<T, I> };
                auto& field_val = field_reflection::get_field<I>(obj);
                using FieldType = std::decay_t<decltype(field_val)>;

                PropertyValue p_val = (*this)[node_id][key];

                if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, int64_t>) {
                    field_val = static_cast<FieldType>(std::get<int64_t>(p_val.value));
                } else if constexpr (std::is_same_v<FieldType, double> || std::is_same_v<FieldType, float>) {
                    field_val = static_cast<FieldType>(std::get<double>(p_val.value));
                } else if constexpr (std::is_same_v<FieldType, std::string>) {
                    field_val = std::get<std::string>(p_val.value);
                } else if constexpr (std::is_same_v<FieldType, bool>) {
                    field_val = std::get<bool>(p_val.value);
                } else if constexpr (std::is_same_v<FieldType, types::SparseTensor>) {
                    field_val = std::get<types::SparseTensor>(p_val.value);
                }
            };
            (deserialize_field(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<field_reflection::field_count<T>>());

        return std::make_optional(obj);
    }


    // 1. SET STRUTTURA ARCO: Salva le proprietà dell'arco e registra il nome letterale della classe
    template <typename T>
    void set_edge_struct(size_t edge_id, const T& obj) {
        static_assert(std::is_aggregate_v<T>, "La riflessione richiede una struct aggregata (POD).");

        // Registra automaticamente il tipo nel registro globale
        PropertyGraphTypeRegistry::instance().register_type<T>();

        // Memorizza il nome stringa unico dell'arco
        std::string e_name{ field_reflection::type_name<T> };
        get_edge(edge_id)["__edge_type_name"] = PropertyValue{ PropertyType::STRING, e_name };

        // Serializzazione riflessa di tutti i campi della struct dell'arco
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            auto serialize_field = [&](auto index_constant) {
                constexpr std::size_t I = decltype(index_constant)::value;
                std::string key{ field_reflection::field_name<T, I> };
                const auto& field_val = field_reflection::get_field<I>(obj);
                using FieldType = std::decay_t<decltype(field_val)>;

                if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, int64_t>) {
                    get_edge(edge_id)[key] = PropertyValue{ PropertyType::INT, int64_t(field_val) };
                } else if constexpr (std::is_same_v<FieldType, double> || std::is_same_v<FieldType, float>) {
                    get_edge(edge_id)[key] = PropertyValue{ PropertyType::DOUBLE, double(field_val) };
                } else if constexpr (std::is_same_v<FieldType, std::string>) {
                    get_edge(edge_id)[key] = PropertyValue{ PropertyType::STRING, field_val };
                } else if constexpr (std::is_same_v<FieldType, bool>) {
                    get_edge(edge_id)[key] = PropertyValue{ PropertyType::BOOL, field_val };
                } else if constexpr (std::is_same_v<FieldType, types::SparseTensor>) {
                    get_edge(edge_id)[key] = PropertyValue{ PropertyType::SPARSE_TENSOR, field_val };
                }
            };
            (serialize_field(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<field_reflection::field_count<T>>());
    }

    // 2. QUERY DEL NOME TIPO ARCO: Estrae la stringa salvata su disco per l'arco
    std::string get_edge_type_string(size_t edge_id) {
        try {
            PropertyValue p_val = get_edge(edge_id)["__edge_type_name"];
            return std::get<std::string>(p_val.value);
        } catch (...) {
            return "";
        }
    }

    // Ritorna std::optional poiche std::type_index non e default-constructible
    std::optional<std::type_index> get_edge_type_index(size_t edge_id) {
        std::string type_str = get_edge_type_string(edge_id);
        if (type_str.empty()) {
            return std::nullopt; // L'arco esiste ma non e associato a nessuna struct riflessa
        }

        // Inizializzazione temporanea con un tipo valido per soddisfare la firma di get_type_index
        std::type_index idx = std::type_index(typeid(void));
        if (PropertyGraphTypeRegistry::instance().get_type_index(type_str, idx)) {
            return idx;
        }
        return std::nullopt; // Tipo non ancora registrato nella sessione corrente
    }

    // 3. VERIFICA COMPATIBILITÀ ARCO: Controlla a runtime se il tipo T coincide
    template <typename T>
    bool is_edge_type_compatible(size_t edge_id) {
        if constexpr (!std::is_aggregate_v<T>) {
            return false;
        }
        std::string stored_name = get_edge_type_string(edge_id);
        std::string requested_name{ field_reflection::type_name<T> };
        return (stored_name == requested_name);
    }

    // 4. RECUPERO SICURO ARCO: Restituisce l'optional popolato solo in assenza di collisioni o mismatch
    template <typename T>
    std::optional<T> try_get_edge_struct(size_t edge_id) {
        if (!is_edge_type_compatible<T>(edge_id)) {
            return std::nullopt;
        }

        T obj{};
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            auto deserialize_field = [&](auto index_constant) {
                constexpr std::size_t I = decltype(index_constant)::value;
                std::string key{ field_reflection::field_name<T, I> };
                auto& field_val = field_reflection::get_field<I>(obj);
                using FieldType = std::decay_t<decltype(field_val)>;

                PropertyValue p_val = get_edge(edge_id)[key];

                if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, int64_t>) {
                    field_val = static_cast<FieldType>(std::get<int64_t>(p_val.value));
                } else if constexpr (std::is_same_v<FieldType, double> || std::is_same_v<FieldType, float>) {
                    field_val = static_cast<FieldType>(std::get<double>(p_val.value));
                } else if constexpr (std::is_same_v<FieldType, std::string>) {
                    field_val = std::get<std::string>(p_val.value);
                } else if constexpr (std::is_same_v<FieldType, bool>) {
                    field_val = std::get<bool>(p_val.value);
                } else if constexpr (std::is_same_v<FieldType, types::SparseTensor>) {
                    field_val = std::get<types::SparseTensor>(p_val.value);
                }
            };
            (deserialize_field(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<field_reflection::field_count<T>>());

        return std::make_optional(obj);
    }

    void commit();
    void rollback();
};

// Manager principale dello Storage su memoria secondaria
class PropertyGraphStore {
friend class Transaction;
private:
    std::fstream nodes_file;
    std::fstream edges_file;
    std::fstream payloads_file;


    std::string serialize_payload(const std::vector<std::string>& labels, const std::unordered_map<std::string, PropertyValue>& props);
    void deserialize_payload(const std::string& buffer, std::vector<std::string>& labels, std::unordered_map<std::string, PropertyValue>& props);
    bool check_node_exists_on_disk(size_t id);
    bool check_edge_exists_on_disk(size_t id);

public:

    size_t get_next_node_id_from_disk();
    size_t get_next_edge_id_from_disk();
    bool fetch_node_from_disk(size_t id, NodeData& data);
    bool fetch_edge_from_disk(size_t id, EdgeData& data);

    std::vector<size_t> get_outgoing_edge_ids(size_t node_id);
    // Estrae tutti gli ID degli archi entranti in un nodo navigando la catena sul disco
    std::vector<size_t> get_ingoing_edge_ids(size_t node_id);

PropertyGraphStore(const std::string& base_path);
    ~PropertyGraphStore();
    Transaction begin_transaction();
};


//
// class PropertyGraphStore;
//
// // --- PROXY PER LE PROPRIETÀ (OPERATOR[] OVERLOAD) ---
// class PropertyProxy {
// private:
//     std::unordered_map<std::string, PropertyValue>& properties_ref;
//     std::string key;
//     bool& dirty_flag_ref;
//
// public:
//     PropertyProxy(std::unordered_map<std::string, PropertyValue>& props, const std::string& k, bool& dirty)
//         : properties_ref(props), key(k), dirty_flag_ref(dirty) {}
//
//     operator PropertyValue() const {
//         auto it = properties_ref.find(key);
//         if (it == properties_ref.end()) {
//             throw std::runtime_error("Errore: Proprietà '" + key + "' inesistente.");
//         }
//         return it->second;
//     }
//
//     PropertyProxy& operator=(const PropertyValue& val) {
//         properties_ref[key] = val;
//         dirty_flag_ref = true;
//         return *this;
//     }
// };
//
// class NodeProxy {
// private:
//     NodeData& data;
//     bool& dirty_flag;
//
// public:
//     NodeProxy(NodeData& d, bool& df) : data(d), dirty_flag(df) {}
//     PropertyProxy operator[](const std::string& key) {
//         return PropertyProxy(data.properties, key, dirty_flag);
//     }
//     std::vector<std::string>& labels() { dirty_flag = true; return data.labels; }
//     size_t id() const { return data.id; }
// };
//
// class EdgeProxy {
// private:
//     EdgeData& data;
//     bool& dirty_flag;
//
// public:
//     EdgeProxy(EdgeData& d, bool& df) : data(d), dirty_flag(df) {}
//     PropertyProxy operator[](const std::string& key) {
//         return PropertyProxy(data.properties, key, dirty_flag);
//     }
//     std::vector<std::string>& labels() { dirty_flag = true; return data.labels; }
//     size_t id() const { return data.id; }
// };
//
// // --- CONTESTO DI TRANSAZIONE ---
// class Transaction {
// friend class PropertyGraphStore;
// private:
//     PropertyGraphStore& store;
//     std::unordered_map<size_t, NodeData> cached_nodes;
//     std::unordered_map<size_t, EdgeData> cached_edges;
//     std::unordered_map<size_t, bool> dirty_nodes;
//     std::unordered_map<size_t, bool> dirty_edges;
//
//     bool node_exists_context(size_t node_id);
//     bool edge_exists_context(size_t edge_id);
//
// public:
//     Transaction(PropertyGraphStore& s) : store(s) {}
//
//     void create_node(size_t node_id);
//     void create_edge(size_t edge_id, size_t source_id, size_t target_id);
//
//     NodeProxy operator[](size_t node_id);
//     EdgeProxy get_edge(size_t edge_id);
//
//     void commit();
//     void rollback() {
//         cached_nodes.clear(); cached_edges.clear();
//         dirty_nodes.clear();  dirty_edges.clear();
//     }
// };
//
// // --- MOTORE DI STORAGE PRINCIPALE ---
// class PropertyGraphStore {
// friend class Transaction;
// private:
//     std::fstream nodes_file;
//     std::fstream edges_file;
//     std::fstream payloads_file;
//
//     std::string serialize_payload(const std::vector<std::string>& labels, const std::unordered_map<std::string, PropertyValue>& props);
//     void deserialize_payload(const std::string& buffer, std::vector<std::string>& labels, std::unordered_map<std::string, PropertyValue>& props);
//     bool check_node_exists_on_disk(size_t id);
//     bool fetch_node_from_disk(size_t id, NodeData& data);
//
// public:
//     PropertyGraphStore(const std::string& base_path);
//     Transaction begin_transaction() { return Transaction(*this); }
// };

}

#endif //PARAMONEUR2_PG_STORE_H
