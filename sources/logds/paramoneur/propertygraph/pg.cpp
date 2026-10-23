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
}