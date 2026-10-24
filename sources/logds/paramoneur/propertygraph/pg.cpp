//
// Created by gyankos on 24/10/26.
//

#include <logds/paramoneur/propertygraph/pg.h>

namespace logds::pg {
    // --- IMPLEMENTAZIONE DEGLI OPERATORI DELLE VISTE ---
    NodeProxy NodeView::operator[](size_t node_id) {
        graph_ref.ensure_transaction();
        return (*graph_ref.active_tx)[node_id];
    }

    EdgeProxy EdgeView::operator[](size_t edge_id) {
        graph_ref.ensure_transaction();
        return graph_ref.active_tx->get_edge(edge_id);
    }

    std::string PropertyGraph::to_graphml() {
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
                        const auto& tensor = std::get<logds::types::SparseTensor>(v.value);
                        ss << "      <data key=\"" << k << "_rank\">" << tensor.get_rank() << "</data>\n";

                        ss << "      <data key=\"" << k << "_shape\">[";
                        for (size_t s = 0; s < tensor.get_shape().size(); ++s) {
                            ss << tensor.get_shape()[s] << (s + 1 < tensor.get_shape().size() ? "," : "");
                        }
                        ss << "]</data>\n";

                        ss << "      <data key=\"" << k << "_default\">";
                        if (tensor.get_data_type() == logds::types::TensorDataType::INT64) ss << std::get<int64_t>(tensor.get_default_value());
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
                        const auto& tensor = std::get<logds::types::SparseTensor>(v.value);
                        ss << "      <data key=\"" << k << "_rank\">" << tensor.get_rank() << "</data>\n";

                        ss << "      <data key=\"" << k << "_shape\">[";
                        const auto& shape = tensor.get_shape();
                        for (size_t s = 0; s < shape.size(); ++s) {
                            ss << shape[s] << (s + 1 < shape.size() ? "," : "");
                        }
                        ss << "]</data>\n";

                        ss << "      <data key=\"" << k << "_default\">";
                        if (tensor.get_data_type() == logds::types::TensorDataType::INT64) ss << std::get<int64_t>(tensor.get_default_value());
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

    std::string PropertyGraph::to_dot() {
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
}
