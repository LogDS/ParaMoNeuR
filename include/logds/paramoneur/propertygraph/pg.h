//
// Created by gyankos on 24/10/26.
//

#include <logds/paramoneur/propertygraph/pg_store.h>
#include <optional>
#include <iostream>
#include <string>
#include <sstream>

#ifndef PARAMONEUR2_PG_H
#define PARAMONEUR2_PG_H

namespace logds::pg {
    class PropertyGraph;

class NodeView {
private:
    PropertyGraph& graph_ref;
public:
    NodeView(PropertyGraph& g) : graph_ref(g) {}

    // Consente la sintassi G.nodes[id]["key"] = value;
    NodeProxy operator[](size_t node_id);
};

class EdgeView {
private:
    PropertyGraph& graph_ref;
public:
    EdgeView(PropertyGraph& g) : graph_ref(g) {}

    // Consente la sintassi G.edges[id]["key"] = value;
    EdgeProxy operator[](size_t edge_id);
};

// --- MEDIATORE PRINCIPALE: PROPERTYGRAPH (STILE NETWORKX) ---
class PropertyGraph {
friend class NodeView;
friend class EdgeView;
private:
    logds::pg::PropertyGraphStore store;
    std::optional<Transaction> active_tx;

    // Forza l'apertura automatica di una transazione se non presente
    void ensure_transaction() {
        if (!active_tx.has_value()) {
            active_tx.emplace(store.begin_transaction());
        }
    }

public:
    NodeView nodes;
    EdgeView edges;

    PropertyGraph(const std::string& base_path)
        : store(base_path), nodes(*this), edges(*this) {}

    ~PropertyGraph() {
        // Guardrail: se l'utente dimentica il commit, esegui il flush al distruttore
        commit();
    }

    // Interfaccia NetworkX: Creazione nodi ed archi con ritorno dell'ID
    size_t add_node() {
        ensure_transaction();
        return active_tx->create_node();
    }

    template <typename T>
    size_t add_node(const T& struct_data) {
        ensure_transaction();
        size_t id = active_tx->create_node();
        active_tx->set_node_struct(id, struct_data);
        return id;
    }

    size_t add_edge(size_t u, size_t v) {
        ensure_transaction();
        return active_tx->create_edge(u, v);
    }

    template <typename T>
    size_t add_edge(size_t u, size_t v, const T& struct_data) {
        ensure_transaction();
        size_t id = active_tx->create_edge(u, v);
        active_tx->set_edge_struct(id, struct_data);
        return id;
    }

    // Navigazione e recupero sicuro tramite Optional riflessi
    template <typename T>
    std::optional<T> get_node_data(size_t node_id) {
        ensure_transaction();
        return active_tx->try_get_node_struct<T>(node_id);
    }

    template <typename T>
    std::optional<T> get_edge_data(size_t edge_id) {
        ensure_transaction();
        return active_tx->try_get_edge_struct<T>(edge_id);
    }

    std::vector<size_t> neighbors(size_t node_id) {
        return successors(node_id); // In un grafo orientato, neighbors mappa i successori
    }

    // --- ADIACENZE NODI (SUCCESSORI / VICINI) ---
    std::vector<size_t> successors(size_t node_id) {
        ensure_transaction();
        std::vector<size_t> node_ids;
        std::vector<size_t> edges = store.get_outgoing_edge_ids(node_id);
        for (size_t e_id : edges) {
            // Sfrutta la fetch_edge_from_disk interna per ricavare il target del record dell'arco
            EdgeData e_data;
            if (store.fetch_edge_from_disk(e_id, e_data)) {
                node_ids.push_back(e_data.target_id);
            }
        }
        return node_ids;
    }


    // --- ADIACENZE NODI (PREDECCESSORI) ---
    std::vector<size_t> predecessors(size_t node_id) {
        ensure_transaction();
        std::vector<size_t> node_ids;
        std::vector<size_t> edges = store.get_ingoing_edge_ids(node_id);
        for (size_t e_id : edges) {
            EdgeData e_data;
            if (store.fetch_edge_from_disk(e_id, e_data)) {
                node_ids.push_back(e_data.source_id);
            }
        }
        return node_ids;
    }

    // --- ADIACENZE ARCHI (OUT EDGES) ---
    std::vector<EdgeTuple> out_edges(size_t node_id) {
        ensure_transaction();
        std::vector<EdgeTuple> result;
        std::vector<size_t> edges = store.get_outgoing_edge_ids(node_id);
        for (size_t e_id : edges) {
            EdgeData e_data;
            if (store.fetch_edge_from_disk(e_id, e_data)) {
                result.push_back(EdgeTuple{e_data.source_id, e_data.target_id, e_id});
            }
        }
        return result;
    }

    // --- ADIACENZE ARCHI (IN EDGES) ---
    std::vector<EdgeTuple> in_edges(size_t node_id) {
        ensure_transaction();
        std::vector<EdgeTuple> result;
        std::vector<size_t> edges = store.get_ingoing_edge_ids(node_id);
        for (size_t e_id : edges) {
            EdgeData e_data;
            if (store.fetch_edge_from_disk(e_id, e_data)) {
                result.push_back(EdgeTuple{e_data.source_id, e_data.target_id, e_id});
            }
        }
        return result;
    }



    // Consolidamento esplicito su disco (Equivalente al salvataggio dello stato)
    void commit() {
        if (active_tx.has_value()) {
            active_tx->commit();
            active_tx.reset(); // Distrugge la transazione e pulisce la cache in RAM
        }
    }

    void rollback() {
        if (active_tx.has_value()) {
            active_tx->rollback();
            active_tx.reset();
        }
    }


public:
    std::string to_graphml() {
        ensure_transaction();
        std::ostringstream ss;

        ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        ss << "<graphml xmlns=\"http://graphdrawing.org\"\n";
        ss << "         xmlns:xsi=\"http://w3.org\"\n";
        ss << "         xsi:schemaLocation=\"http://graphdrawing.org/1.0/graphml.xsd\">\n";

        // Mappa per memorizzare in modo speculare l'ID esatto e il tipo di dato per l'header
        std::unordered_map<std::string, std::string> node_keys, edge_keys;
        node_keys["__type_name"] = "string";
        edge_keys["__edge_type_name"] = "string";

        // Pre-scansione simmetrica dei nodi per mappare tutte le chiavi reali presenti
        size_t total_nodes = store.get_next_node_id_from_disk();
        for (size_t i = 0; i < total_nodes; ++i) {
            logds::pg::NodeData nd;
            if (store.fetch_node_from_disk(i, nd)) {
                for (const auto& [k, v] : nd.properties) {
                    if (v.type == logds::pg::PropertyType::SPARSE_TENSOR) {
                        node_keys[k + "_rank"] = "integer";
                        node_keys[k + "_shape"] = "string";
                        node_keys[k + "_default"] = "double";
                        node_keys[k + "_coo_data"] = "string";
                    } else {
                        node_keys[k] = "string";
                    }
                }
            }
        }

        // Pre-scansione simmetrica degli archi per mappare tutte le chiavi reali presenti
        size_t total_edges = store.get_next_edge_id_from_disk();
        for (size_t i = 0; i < total_edges; ++i) {
            logds::pg::EdgeData ed;
            if (store.fetch_edge_from_disk(i, ed)) {
                for (const auto& [k, v] : ed.properties) {
                    if (v.type == logds::pg::PropertyType::SPARSE_TENSOR) {
                        edge_keys[k + "_rank"] = "integer";
                        edge_keys[k + "_shape"] = "string";
                        edge_keys[k + "_default"] = "double";
                        edge_keys[k + "_coo_data"] = "string";
                    } else {
                        edge_keys[k] = "string";
                    }
                }
            }
        }

        // 1. GENERAZIONE HEADER: Scrittura delle chiavi con ID pulito e univoco
        for (const auto& [k, t] : node_keys) {
            ss << "  <key id=\"" << k << "\" for=\"node\" attr.name=\"" << k << "\" attr.type=\"" << t << "\"/>\n";
        }
        for (const auto& [k, t] : edge_keys) {
            ss << "  <key id=\"" << k << "\" for=\"edge\" attr.name=\"" << k << "\" attr.type=\"" << t << "\"/>\n";
        }

        ss << "  <graph id=\"G\" edgedefault=\"directed\">\n";

        // 2. GENERAZIONE DATI NODI: L'attributo key fa match esatto con l'id dell'header
        for (size_t i = 0; i < total_nodes; ++i) {
            logds::pg::NodeData nd;
            if (store.fetch_node_from_disk(i, nd)) {
                ss << "    <node id=\"n" << i << "\">\n";

                std::string t_name = active_tx->get_node_type_string(i);
                if (!t_name.empty()) {
                    ss << "      <data key=\"__type_name\">" << t_name << "</data>\n";
                }

                for (const auto& [k, v] : nd.properties) {
                    if (k == "__type_name" || k == "__type_hash") continue;

                    if (v.type == logds::pg::PropertyType::SPARSE_TENSOR) {
                        const auto& tensor = std::get<logds::pg::SparseTensor>(v.value);
                        ss << "      <data key=\"" << k << "_rank\">" << tensor.get_rank() << "</data>\n";

                        ss << "      <data key=\"" << k << "_shape\">[";
                        for (size_t s = 0; s < tensor.get_shape().size(); ++s) {
                            ss << tensor.get_shape()[s] << (s + 1 < tensor.get_shape().size() ? "," : "");
                        }
                        ss << "]</data>\n";

                        ss << "      <data key=\"" << k << "_default\">";
                        if (tensor.get_data_type() == logds::pg::TensorDataType::INT64) ss << std::get<int64_t>(tensor.get_default_value());
                        else ss << std::get<double>(tensor.get_default_value());
                        ss << "</data>\n";

                        ss << "      <data key=\"" << k << "_coo_data\"><![CDATA[" << serialize_tensor_coo_clean(tensor) << "]]></data>\n";
                    } else {
                        ss << "      <data key=\"" << k << "\">" << format_simple_value(v, true) << "</data>\n";
                    }
                }
                ss << "    </node>\n";
            }
        }

        // 3. GENERAZIONE DATI ARCHI: Match perfetto delle chiavi
        for (size_t i = 0; i < total_edges; ++i) {
            logds::pg::EdgeData ed;
            if (store.fetch_edge_from_disk(i, ed)) {
                ss << "    <edge id=\"e" << i << "\" source=\"n" << ed.source_id
                   << "\" target=\"n" << ed.target_id << "\">\n";

                std::string t_name = active_tx->get_edge_type_string(i);
                if (!t_name.empty()) {
                    ss << "      <data key=\"__edge_type_name\">" << t_name << "</data>\n";
                }

                for (const auto& [k, v] : ed.properties) {
                    if (k == "__edge_type_name") continue;

                    if (v.type == logds::pg::PropertyType::SPARSE_TENSOR) {
                        const auto& tensor = std::get<logds::pg::SparseTensor>(v.value);
                        ss << "      <data key=\"" << k << "_rank\">" << tensor.get_rank() << "</data>\n";

                        ss << "      <data key=\"" << k << "_shape\">[";
                        const auto& shape = tensor.get_shape();
                        for (size_t s = 0; s < shape.size(); ++s) {
                            ss << shape[s] << (s + 1 < shape.size() ? "," : "");
                        }
                        ss << "]</data>\n";

                        ss << "      <data key=\"" << k << "_default\">";
                        if (tensor.get_data_type() == logds::pg::TensorDataType::INT64) ss << std::get<int64_t>(tensor.get_default_value());
                        else ss << std::get<double>(tensor.get_default_value());
                        ss << "</data>\n";

                        ss << "      <data key=\"" << k << "_coo_data\"><![CDATA[" << serialize_tensor_coo_clean(tensor) << "]]></data>\n";
                    } else {
                        ss << "      <data key=\"" << k << "\">" << format_simple_value(v, true) << "</data>\n";
                    }
                }
                ss << "    </edge>\n";
            }
        }

        ss << "  </graph>\n";
        ss << "</graphml>\n";
        return ss.str();
    }


    std::string to_dot();

private:
    // 1. HELPER DI SANITIZZAZIONE E CONVERSIONE DEI TIPI PRIMITIVI SCALARI
    std::string format_simple_value(const logds::pg::PropertyValue& p_val, bool is_xml) {
        std::ostringstream oss;
        if (p_val.type == logds::pg::PropertyType::INT) {
            oss << std::get<int64_t>(p_val.value);
        } else if (p_val.type == logds::pg::PropertyType::DOUBLE) {
            oss << std::get<double>(p_val.value);
        } else if (p_val.type == logds::pg::PropertyType::BOOL) {
            oss << (std::get<bool>(p_val.value) ? "true" : "false");
        } else if (p_val.type == logds::pg::PropertyType::STRING) {
            std::string str = std::get<std::string>(p_val.value);
            for (char c : str) {
                if (is_xml) {
                    // Escape standard dei caratteri speciali per non rompere i tag XML di GraphML
                    if (c == '<') oss << "&lt;";
                    else if (c == '>') oss << "&gt;";
                    else if (c == '&') oss << "&amp;";
                    else if (c == '"') oss << "&quot;";
                    else oss << c;
                } else {
                    // Escape standard dei caratteri speciali per le label HTML-like di Graphviz
                    if (c == '<') oss << "&lt;";
                    else if (c == '>') oss << "&gt;";
                    else if (c == '&') oss << "&amp;";
                    else if (c == '"') oss << "&quot;";
                    else oss << c;
                }
            }
        }
        return oss.str();
    }

    // 2. HELPER DI SERIALIZZAZIONE DELLE MATRICI COO IN FORMATO JSON PIATTO PULITO
    std::string serialize_tensor_coo_clean(const logds::pg::SparseTensor& tensor) {
        std::ostringstream oss;
        oss << "[";
        const auto& coordinates = tensor.get_coordinates();
        const auto& dt = tensor.get_data_type();
        const auto& values = tensor.get_values();
        for (size_t i = 0; i < coordinates.size(); ++i) {
            oss << "{\"c\":[";
            for (size_t r = 0; r <  coordinates[i].size(); ++r) {
                oss <<  coordinates[i][r] << (r + 1 <  coordinates[i].size() ? "," : "");
            }
            oss << "],\"v\":";
            if (dt == logds::pg::TensorDataType::INT64) {
                oss << std::get<std::vector<int64_t>>(values)[i];
            } else {
                oss << std::get<std::vector<double>>(values)[i];
            }
            oss << "}" << (i + 1 < coordinates.size() ? "," : "");
        }
        oss << "]";
        return oss.str();
    }


    // Helper modificato per generare direttamente righe di tabella <TR><TD> verticali per il formato DOT
    std::string format_property_as_html_rows(const std::string& key, const logds::pg::PropertyValue& p_val) {
        std::ostringstream oss;

        if (p_val.type == logds::pg::PropertyType::INT) {
            oss << "      <TR><TD ALIGN=\"LEFT\"><B>" << key << ":</B> " << std::get<int64_t>(p_val.value) << "</TD></TR>\n";
        }
        else if (p_val.type == logds::pg::PropertyType::DOUBLE) {
            oss << "      <TR><TD ALIGN=\"LEFT\"><B>" << key << ":</B> " << std::get<double>(p_val.value) << "</TD></TR>\n";
        }
        else if (p_val.type == logds::pg::PropertyType::BOOL) {
            oss << "      <TR><TD ALIGN=\"LEFT\"><B>" << key << ":</B> " << (std::get<bool>(p_val.value) ? "true" : "false") << "</TD></TR>\n";
        }
        else if (p_val.type == logds::pg::PropertyType::STRING) {
            std::string str = std::get<std::string>(p_val.value);
            oss << "      <TR><TD ALIGN=\"LEFT\"><B>" << key << ":</B> ";
            for (char c : str) {
                if (c == '<') oss << "&lt;";
                else if (c == '>') oss << "&gt;";
                else if (c == '&') oss << "&amp;";
                else if (c == '"') oss << "&quot;";
                else oss << c;
            }
            oss << "</TD></TR>\n";
        }
        else if (p_val.type == logds::pg::PropertyType::SPARSE_TENSOR) {
            const auto& tensor = std::get<logds::pg::SparseTensor>(p_val.value);

            // Creiamo una sezione ad impilamento verticale interna dedicata al Tensore
            oss << "      <TR><TD BGCOLOR=\"#F3F4F6\" ALIGN=\"LEFT\"><B>" << key << " (SparseTensor)</B></TD></TR>\n";

            // Riga per il Rango
            oss << "      <TR><TD ALIGN=\"LEFT\">  <I>rank:</I> " << tensor.get_rank() << "</TD></TR>\n";

            // Riga per la Shape
            oss << "      <TR><TD ALIGN=\"LEFT\">  <I>shape:</I> [";
            const auto& shape = tensor.get_shape();
            for (size_t s = 0; s < shape.size(); ++s) {
                oss << shape[s] << (s + 1 < shape.size() ? "," : "");
            }
            oss << "]</TD></TR>\n";

            // Riga per il valore di Background (Default)
            oss << "      <TR><TD ALIGN=\"LEFT\">  <I>default:</I> ";
            if (tensor.get_data_type() == logds::pg::TensorDataType::INT64) {
                oss << std::get<int64_t>(tensor.get_default_value());
            } else {
                oss << std::get<double>(tensor.get_default_value());
            }
            oss << "</TD></TR>\n";

            // Righe verticali separate per ciascun elemento non nullo del formato COO
            const auto& coordinates = tensor.get_coordinates();
            if (!coordinates.empty()) {
                oss << "      <TR><TD BGCOLOR=\"#F9FAFB\" ALIGN=\"LEFT\"><I>Non-Zero Elements:</I></TD></TR>\n";
                for (size_t i = 0; i < coordinates.size(); ++i) {
                    oss << "      <TR><TD ALIGN=\"LEFT\">    coor [";
                    for (size_t r = 0; r < coordinates[i].size(); ++r) {
                        oss << coordinates[i][r] << (r + 1 < coordinates[i].size() ? "," : "");
                    }
                    oss << "] = <B>";
                    if (tensor.get_data_type() == logds::pg::TensorDataType::INT64) {
                        oss << std::get<std::vector<int64_t>>(tensor.get_values())[i];
                    } else {
                        oss << std::get<std::vector<double>>(tensor.get_values())[i];
                    }
                    oss << "</B></TD></TR>\n";
                }
            }
        }
        return oss.str();
    }

std::string format_property_value(const logds::pg::PropertyValue& p_val, bool is_xml) {
        std::ostringstream oss;
        if (p_val.type == logds::pg::PropertyType::INT) {
            oss << std::get<int64_t>(p_val.value);
        } else if (p_val.type == logds::pg::PropertyType::DOUBLE) {
            oss << std::get<double>(p_val.value);
        } else if (p_val.type == logds::pg::PropertyType::BOOL) {
            oss << (std::get<bool>(p_val.value) ? "true" : "false");
        } else if (p_val.type == logds::pg::PropertyType::STRING) {
            std::string str = std::get<std::string>(p_val.value);
            for (char c : str) {
                if (is_xml) {
                    if (c == '<') oss << "&lt;";
                    else if (c == '>') oss << "&gt;";
                    else if (c == '&') oss << "&amp;";
                    else if (c == '"') oss << "&quot;";
                    else oss << c;
                } else {
                    // All'interno dei tag HTML di Graphviz, <, >, & vanno convertiti in entità HTML
                    if (c == '<') oss << "&lt;";
                    else if (c == '>') oss << "&gt;";
                    else if (c == '&') oss << "&amp;";
                    else if (c == '"') oss << "&quot;";
                    else oss << c;
                }
            }
        } else if (p_val.type == logds::pg::PropertyType::SPARSE_TENSOR) {
            const auto& tensor = std::get<logds::pg::SparseTensor>(p_val.value);

            // Usiamo una formattazione multilinea pulita per una visualizzazione ottimale
            std::string separator = is_xml ? " | " : "<BR/>";

            oss << "Tensor(rank=" << tensor.get_rank() << ", shape=[";
            for (size_t s = 0; s < tensor.get_shape().size(); ++s) {
                oss << tensor.get_shape()[s] << (s + 1 < tensor.get_shape().size() ? "," : "");
            }
            oss << "], default=";
            if (tensor.get_data_type() == logds::pg::TensorDataType::INT64) {
                oss << std::get<int64_t>(tensor.get_default_value());
            } else {
                oss << std::get<double>(tensor.get_default_value());
            }
            oss << ")" << separator << "Elements:";

            for (size_t i = 0; i < tensor.get_coordinates().size(); ++i) {
                oss << separator << "  [";
                for (size_t r = 0; r < tensor.get_coordinates()[i].size(); ++r) {
                    oss << tensor.get_coordinates()[i][r] << (r + 1 < tensor.get_coordinates()[i].size() ? "," : "");
                }
                oss << "] = ";
                if (tensor.get_data_type() == logds::pg::TensorDataType::INT64) {
                    oss << std::get<std::vector<int64_t>>(tensor.get_values())[i];
                } else {
                    oss << std::get<std::vector<double>>(tensor.get_values())[i];
                }
            }
        }
        return oss.str();
    }

};
}

#endif //PARAMONEUR2_PG_H
