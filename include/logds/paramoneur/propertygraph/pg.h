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


    std::string to_graphml() {
        ensure_transaction();
        std::ostringstream ss;

        ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        ss << "<graphml xmlns=\"http://graphdrawing.org\"\n";
        ss << "         xmlns:xsi=\"http://w3.org\"\n";
        ss << "         xsi:schemaLocation=\"http://graphdrawing.org/1.0/graphml.xsd\">\n";

        // Mappa per raccogliere globalmente tutte le chiavi di proprietà per dichiararle nell'header
        std::unordered_map<std::string, std::string> node_keys, edge_keys;
        node_keys["__type_name"] = "string";
        edge_keys["__edge_type_name"] = "string";

        size_t total_nodes = store.get_next_node_id_from_disk();
        for (size_t i = 0; i < total_nodes; ++i) {
            logds::pg::NodeData nd;
            if (store.fetch_node_from_disk(i, nd)) {
                for (const auto& [k, v] : nd.properties) node_keys[k] = "string";
            }
        }
        size_t total_edges = store.get_next_edge_id_from_disk();
        for (size_t i = 0; i < total_edges; ++i) {
            logds::pg::EdgeData ed;
            if (store.fetch_edge_from_disk(i, ed)) {
                for (const auto& [k, v] : ed.properties) edge_keys[k] = "string";
            }
        }

        // Scrittura dei tag <key> nell'header XML
        for (const auto& [k, t] : node_keys) {
            ss << "  <key id=\"n_" << k << "\" for=\"node\" attr.name=\"" << k << "\" attr.type=\"" << t << "\"/>\n";
        }
        for (const auto& [k, t] : edge_keys) {
            ss << "  <key id=\"e_" << k << "\" for=\"edge\" attr.name=\"" << k << "\" attr.type=\"" << t << "\"/>\n";
        }

        ss << "  <graph id=\"G\" edgedefault=\"directed\">\n";

        // Scrittura Nodi con srotolamento completo delle proprietà
        for (size_t i = 0; i < total_nodes; ++i) {
            logds::pg::NodeData nd;
            if (store.fetch_node_from_disk(i, nd)) {
                ss << "    <node id=\"n" << i << "\">\n";
                std::string t_name = active_tx->get_node_type_string(i);
                if (!t_name.empty()) {
                    ss << "      <data key=\"n___type_name\">" << t_name << "</data>\n";
                }
                for (const auto& [k, v] : nd.properties) {
                    if (k == "__type_name" || k == "__type_hash") continue;
                    ss << "      <data key=\"n_" << k << "\">" << format_property_value(v, true) << "</data>\n";
                }
                ss << "    </node>\n";
            }
        }

        // Scrittura Archi con srotolamento completo delle proprietà
        for (size_t i = 0; i < total_edges; ++i) {
            logds::pg::EdgeData ed;
            if (store.fetch_edge_from_disk(i, ed)) {
                ss << "    <edge id=\"e" << i << "\" source=\"n" << ed.source_id
                   << "\" target=\"n" << ed.target_id << "\">\n";
                std::string t_name = active_tx->get_edge_type_string(i);
                if (!t_name.empty()) {
                    ss << "      <data key=\"e___edge_type_name\">" << t_name << "</data>\n";
                }
                for (const auto& [k, v] : ed.properties) {
                    if (k == "__edge_type_name") continue;
                    ss << "      <data key=\"e_" << k << "\">" << format_property_value(v, true) << "</data>\n";
                }
                ss << "    </edge>\n";
            }
        }

        ss << "  </graph>\n";
        ss << "</graphml>\n";
        return ss.str();
    }

    std::string to_dot() {
        ensure_transaction();
        std::ostringstream ss;

        ss << "digraph PropertyGraph {\n";
        ss << "  graph [rankdir=LR, overlap=false, splines=true, nodesep=0.6, ranksep=1.2];\n";
        ss << "  node [shape=none, fontname=\"Helvetica\", fontsize=10];\n";
        ss << "  edge [fontname=\"Helvetica\", fontsize=9, color=\"#6B7280\", labelfontcolor=\"#374151\"];\n\n";

        // --- GENERAZIONE DEI NODI VERTICALMENTE IMPILATI ---
        size_t total_nodes = store.get_next_node_id_from_disk();
        for (size_t i = 0; i < total_nodes; ++i) {
            logds::pg::NodeData nd;
            if (store.fetch_node_from_disk(i, nd)) {
                std::string type_name = active_tx->get_node_type_string(i);
                if (type_name.empty()) type_name = "DynamicNode";

                ss << "  " << i << " [label=<\n";
                ss << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" BGCOLOR=\"#F9FAFB\" COLOR=\"#D1D5DB\" PORT=\"f0\">\n";
                ss << "      <TR><TD BGCOLOR=\"#EEF2F6\"><B>Node ID: " << i << "</B></TD></TR>\n";
                ss << "      <TR><TD BGCOLOR=\"#F3F4F6\"><I>Class: " << type_name << "</I></TD></TR>\n";

                // Generazione di ciascuna proprietà su una riga verticale autonoma della tabella HTML
                for (const auto& [k, v] : nd.properties) {
                    if (k == "__type_name" || k == "__type_hash") continue;
                    ss << format_property_as_html_rows(k, v);
                }

                ss << "    </TABLE>\n";
                ss << "  >];\n";
            }
        }

        // --- GENERAZIONE DEGLI ARCHI ---
        size_t total_edges = store.get_next_edge_id_from_disk();
        for (size_t i = 0; i < total_edges; ++i) {
            logds::pg::EdgeData ed;
            if (store.fetch_edge_from_disk(i, ed)) {
                std::string type_name = active_tx->get_edge_type_string(i);
                if (type_name.empty()) type_name = "DynamicEdge";

                std::ostringstream edge_labels;
                edge_labels << "<B>[Edge " << i << ": " << type_name << "]</B>";
                for (const auto& [k, v] : ed.properties) {
                    if (k == "__edge_type_name") continue;

                    // Estratto minimale pulito per le linee di testo dell'arco
                    std::string val_clean;
                    if (v.type == logds::pg::PropertyType::STRING) val_clean = std::get<std::string>(v.value);
                    else if (v.type == logds::pg::PropertyType::INT) val_clean = std::to_string(std::get<int64_t>(v.value));
                    else if (v.type == logds::pg::PropertyType::DOUBLE) val_clean = std::to_string(std::get<double>(v.value));
                    else if (v.type == logds::pg::PropertyType::BOOL) val_clean = std::get<bool>(v.value) ? "true" : "false";

                    edge_labels << "\\n" << k << " = " << val_clean;
                }

                ss << "  " << ed.source_id << " -> " << ed.target_id
                   << " [label=<\n    " << edge_labels.str() << "\n  >];\n";
            }
        }

        ss << "}\n";
        return ss.str();
        // ensure_transaction();
        // std::ostringstream ss;
        //
        // ss << "digraph PropertyGraph {\n";
        // ss << "  graph [rankdir=LR, overlap=false, splines=true, nodesep=0.5, ranksep=1.0];\n";
        // ss << "  node [shape=Mrecord, style=\"filled,rounded\", fillcolor=\"#F9FAFB\", color=\"#D1D5DB\", fontname=\"Helvetica\", fontsize=10];\n";
        // ss << "  edge [fontname=\"Helvetica\", fontsize=9, color=\"#6B7280\", labelfontcolor=\"#374151\"];\n\n";
        //
        // // --- NODI CON ATTRIBUTI COMPLETI ---
        // size_t total_nodes = store.get_next_node_id_from_disk();
        // for (size_t i = 0; i < total_nodes; ++i) {
        //     logds::pg::NodeData nd;
        //     if (store.fetch_node_from_disk(i, nd)) {
        //         std::string type_name = active_tx->get_node_type_string(i);
        //         if (type_name.empty()) type_name = "DynamicNode";
        //
        //         // Inizio Label a Record Multilinea
        //         ss << "  " << i << " [label=\"{ ID: " << i << " | Class: " << type_name;
        //
        //         for (const auto& [k, v] : nd.properties) {
        //             if (k == "__type_name" || k == "__type_hash") continue;
        //             ss << " | " << k << ": " << format_property_value(v, false);
        //         }
        //         ss << " }\"];\n";
        //     }
        // }
        //
        // // --- ARCHI CON ATTRIBUTI COMPLETI ---
        // size_t total_edges = store.get_next_edge_id_from_disk();
        // for (size_t i = 0; i < total_edges; ++i) {
        //     logds::pg::EdgeData ed;
        //     if (store.fetch_edge_from_disk(i, ed)) {
        //         std::string type_name = active_tx->get_edge_type_string(i);
        //         if (type_name.empty()) type_name = "DynamicEdge";
        //
        //         std::ostringstream edge_labels;
        //         edge_labels << "[Edge " << i << ": " << type_name << "]";
        //         for (const auto& [k, v] : ed.properties) {
        //             if (k == "__edge_type_name") continue;
        //             edge_labels << "\\n" << k << "= " << format_property_value(v, false);
        //         }
        //
        //         ss << "  " << ed.source_id << " -> " << ed.target_id
        //            << " [label=\"" << edge_labels.str() << "\"];\n";
        //     }
        // }
        //
        // ss << "}\n";
        // return ss.str();
    }

private:
    private:

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
            oss << "      <TR><TD ALIGN=\"LEFT\">  <I>rank:</I> " << tensor.rank << "</TD></TR>\n";

            // Riga per la Shape
            oss << "      <TR><TD ALIGN=\"LEFT\">  <I>shape:</I> [";
            for (size_t s = 0; s < tensor.shape.size(); ++s) {
                oss << tensor.shape[s] << (s + 1 < tensor.shape.size() ? "," : "");
            }
            oss << "]</TD></TR>\n";

            // Riga per il valore di Background (Default)
            oss << "      <TR><TD ALIGN=\"LEFT\">  <I>default:</I> ";
            if (tensor.data_type == logds::pg::TensorDataType::INT64) {
                oss << std::get<int64_t>(tensor.default_value);
            } else {
                oss << std::get<double>(tensor.default_value);
            }
            oss << "</TD></TR>\n";

            // Righe verticali separate per ciascun elemento non nullo del formato COO
            if (!tensor.coordinates.empty()) {
                oss << "      <TR><TD BGCOLOR=\"#F9FAFB\" ALIGN=\"LEFT\"><I>Non-Zero Elements:</I></TD></TR>\n";
                for (size_t i = 0; i < tensor.coordinates.size(); ++i) {
                    oss << "      <TR><TD ALIGN=\"LEFT\">    coor [";
                    for (size_t r = 0; r < tensor.coordinates[i].size(); ++r) {
                        oss << tensor.coordinates[i][r] << (r + 1 < tensor.coordinates[i].size() ? "," : "");
                    }
                    oss << "] = <B>";
                    if (tensor.data_type == logds::pg::TensorDataType::INT64) {
                        oss << std::get<std::vector<int64_t>>(tensor.values)[i];
                    } else {
                        oss << std::get<std::vector<double>>(tensor.values)[i];
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

            oss << "Tensor(rank=" << tensor.rank << ", shape=[";
            for (size_t s = 0; s < tensor.shape.size(); ++s) {
                oss << tensor.shape[s] << (s + 1 < tensor.shape.size() ? "," : "");
            }
            oss << "], default=";
            if (tensor.data_type == logds::pg::TensorDataType::INT64) {
                oss << std::get<int64_t>(tensor.default_value);
            } else {
                oss << std::get<double>(tensor.default_value);
            }
            oss << ")" << separator << "Elements:";

            for (size_t i = 0; i < tensor.coordinates.size(); ++i) {
                oss << separator << "  [";
                for (size_t r = 0; r < tensor.coordinates[i].size(); ++r) {
                    oss << tensor.coordinates[i][r] << (r + 1 < tensor.coordinates[i].size() ? "," : "");
                }
                oss << "] = ";
                if (tensor.data_type == logds::pg::TensorDataType::INT64) {
                    oss << std::get<std::vector<int64_t>>(tensor.values)[i];
                } else {
                    oss << std::get<std::vector<double>>(tensor.values)[i];
                }
            }
        }
        return oss.str();
    }

};
}

#endif //PARAMONEUR2_PG_H
