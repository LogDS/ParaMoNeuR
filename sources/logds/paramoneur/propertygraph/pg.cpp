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
