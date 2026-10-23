//
// Created by gyankos on 23/10/26.
//

#include "logds/paramoneur/propertygraph/pg_store.h"
#include <sstream>
#include <stdexcept>


namespace logds::pg {
    // --- IMPLEMENTAZIONE PROPERTY PROXY ---
    PropertyProxy::PropertyProxy(std::unordered_map<std::string, PropertyValue> &props, const std::string &k,
                                 bool &dirty)
        : properties_ref(props), key(k), dirty_flag_ref(dirty) {
    }

    PropertyProxy::operator PropertyValue() const {
        auto it = properties_ref.find(key);
        if (it == properties_ref.end()) {
            throw std::runtime_error("Property Error: Property '" + key + "' not found.");
        }
        return it->second;
    }



    PropertyProxy &PropertyProxy::operator=(const PropertyValue &val) {
        properties_ref[key] = val;
        dirty_flag_ref = true;
        return *this;
    }

    // --- IMPLEMENTAZIONE NODE PROXY ---
    NodeProxy::NodeProxy(NodeData &d, bool &df) : data(d), dirty_flag(df) {
    }

    PropertyProxy NodeProxy::operator[](const std::string &key) {
        return PropertyProxy(data.properties, key, dirty_flag);
    }

    std::vector<std::string> &NodeProxy::labels() {
        dirty_flag = true;
        return data.labels;
    }

    size_t NodeProxy::id() const { return data.id; }

    // --- IMPLEMENTAZIONE EDGE PROXY ---
    EdgeProxy::EdgeProxy(EdgeData &d, bool &df) : data(d), dirty_flag(df) {
    }

    PropertyProxy EdgeProxy::operator[](const std::string &key) {
        return PropertyProxy(data.properties, key, dirty_flag);
    }

    std::vector<std::string> &EdgeProxy::labels() {
        dirty_flag = true;
        return data.labels;
    }

    size_t EdgeProxy::id() const { return data.id; }

    // --- IMPLEMENTAZIONE TRANSAZIONE ---
    Transaction::Transaction(PropertyGraphStore &s) : store(s) {
        next_node_id_local = store.get_next_node_id_from_disk();
        next_edge_id_local = store.get_next_edge_id_from_disk();
    }

    bool Transaction::node_exists_context(size_t node_id) {
        if (cached_nodes.find(node_id) != cached_nodes.end()) return true;
        return store.check_node_exists_on_disk(node_id);
    }

    bool Transaction::edge_exists_context(size_t edge_id) {
        if (cached_edges.find(edge_id) != cached_edges.end()) return true;
        return store.check_edge_exists_on_disk(edge_id);
    }

    size_t Transaction::create_node() {
        size_t generated_id = next_node_id_local++;
        NodeData new_node;
        new_node.id = generated_id;
        cached_nodes[generated_id] = new_node;
        dirty_nodes[generated_id] = true;
        return generated_id; // Ritorna l'id incrementale all'utente
    }

    size_t Transaction::create_edge(size_t source_id, size_t target_id) {
        if (!node_exists_context(source_id) || !node_exists_context(target_id)) {
            throw std::runtime_error("Topology Error: Cannot create edge. Source or Target node does not exist.");
        }
        size_t generated_id = next_edge_id_local++;
        EdgeData new_edge;
        new_edge.id = generated_id;
        new_edge.source_id = source_id;
        new_edge.target_id = target_id;
        cached_edges[generated_id] = new_edge;
        dirty_edges[generated_id] = true;
        return generated_id; // Ritorna l'id incrementale all'utente
    }

    NodeProxy Transaction::operator[](size_t node_id) {
        if (cached_nodes.find(node_id) == cached_nodes.end()) {
            NodeData data;
            if (!store.fetch_node_from_disk(node_id, data)) {
                throw std::runtime_error("Access Denied: Node " + std::to_string(node_id) + " does not exist.");
            }
            cached_nodes[node_id] = data;
            dirty_nodes[node_id] = false;
        }
        return NodeProxy(cached_nodes[node_id], dirty_nodes[node_id]);
    }

    EdgeProxy Transaction::get_edge(size_t edge_id) {
        if (cached_edges.find(edge_id) == cached_edges.end()) {
            EdgeData data;
            if (!store.fetch_edge_from_disk(edge_id, data)) {
                throw std::runtime_error("Access Denied: Edge " + std::to_string(edge_id) + " does not exist.");
            }
            cached_edges[edge_id] = data;
            dirty_edges[edge_id] = false;
        }
        return EdgeProxy(cached_edges[edge_id], dirty_edges[edge_id]);
    }

    void Transaction::rollback() {
        cached_nodes.clear();
        cached_edges.clear();
        dirty_nodes.clear();
        dirty_edges.clear();
        next_node_id_local = store.get_next_node_id_from_disk();
        next_edge_id_local = store.get_next_edge_id_from_disk();
    }

    void Transaction::commit() {
        // 1. Scrittura dei Nodi modificati
        for (const auto &[node_id, is_dirty]: dirty_nodes) {
            if (is_dirty) {
                const auto &node_data = cached_nodes[node_id];
                std::string binary = store.serialize_payload(node_data.labels, node_data.properties);

                store.payloads_file.seekp(0, std::ios::end);
                uint64_t offset = store.payloads_file.tellp();
                store.payloads_file.write(binary.data(), binary.size());

                store.nodes_file.seekg(node_id * sizeof(NodeRecord), std::ios::beg);
                NodeRecord rec;
                if (!store.nodes_file.read(reinterpret_cast<char *>(&rec), sizeof(NodeRecord))) {
                    store.nodes_file.clear();
                    rec = NodeRecord();
                }
                rec.exists = true;
                rec.payload_offset = offset;
                rec.payload_size = binary.size();

                store.nodes_file.seekp(node_id * sizeof(NodeRecord), std::ios::beg);
                store.nodes_file.write(reinterpret_cast<const char *>(&rec), sizeof(NodeRecord));
            }
        }

        // 2. Scrittura degli Archi modificati e allineamento liste concatenate
        for (const auto &[edge_id, is_dirty]: dirty_edges) {
            if (is_dirty) {
                const auto &edge_data = cached_edges[edge_id];
                std::string binary = store.serialize_payload(edge_data.labels, edge_data.properties);

                store.payloads_file.seekp(0, std::ios::end);
                uint64_t offset = store.payloads_file.tellp();
                store.payloads_file.write(binary.data(), binary.size());

                store.nodes_file.seekg(edge_data.source_id * sizeof(NodeRecord), std::ios::beg);
                NodeRecord src_rec;
                store.nodes_file.read(reinterpret_cast<char *>(&src_rec), sizeof(NodeRecord));

                store.nodes_file.seekg(edge_data.target_id * sizeof(NodeRecord), std::ios::beg);
                NodeRecord tgt_rec;
                store.nodes_file.read(reinterpret_cast<char *>(&tgt_rec), sizeof(NodeRecord));

                EdgeRecord edge_rec;
                edge_rec.exists = true;
                edge_rec.source_id = edge_data.source_id;
                edge_rec.target_id = edge_data.target_id;
                edge_rec.payload_offset = offset;
                edge_rec.payload_size = binary.size();

                edge_rec.next_outgoing = src_rec.first_outgoing;
                edge_rec.next_ingoing = tgt_rec.first_ingoing;

                src_rec.first_outgoing = edge_id;
                tgt_rec.first_ingoing = edge_id;

                store.edges_file.seekp(edge_id * sizeof(EdgeRecord), std::ios::beg);
                store.edges_file.write(reinterpret_cast<const char *>(&edge_rec), sizeof(EdgeRecord));

                store.nodes_file.seekp(edge_data.source_id * sizeof(NodeRecord), std::ios::beg);
                store.nodes_file.write(reinterpret_cast<const char *>(&src_rec), sizeof(NodeRecord));

                store.nodes_file.seekp(edge_data.target_id * sizeof(NodeRecord), std::ios::beg);
                store.nodes_file.write(reinterpret_cast<const char *>(&tgt_rec), sizeof(NodeRecord));
            }
        }
        store.edges_file.flush();
        store.nodes_file.flush();
        store.payloads_file.flush();

        // Ripristina lo stato interno degli stream (cancella flag di EOF o failure dovuti ai seek)
        store.nodes_file.clear();
        store.edges_file.clear();
        store.payloads_file.clear();
        rollback();
    }

    // --- IMPLEMENTAZIONE ENGINE DI STORAGE ---
    PropertyGraphStore::PropertyGraphStore(const std::string &base_path) {
        auto flags = std::ios::in | std::ios::out | std::ios::binary; {
            // Garanzia di pre-creazione fisica dei file vuoti per evitare crash in apertura
            std::ofstream n(base_path + "_nodes.idx", std::ios::binary | std::ios::app);
            std::ofstream e(base_path + "_edges.idx", std::ios::binary | std::ios::app);
            std::ofstream p(base_path + "_payloads.bin", std::ios::binary | std::ios::app);
        }

        nodes_file.open(base_path + "_nodes.idx", flags);
        edges_file.open(base_path + "_edges.idx", flags);
        payloads_file.open(base_path + "_payloads.bin", flags);
    }

    PropertyGraphStore::~PropertyGraphStore() {
        if (nodes_file.is_open()) nodes_file.close();
        if (edges_file.is_open()) edges_file.close();
        if (payloads_file.is_open()) payloads_file.close();
    }

    Transaction PropertyGraphStore::begin_transaction() { return Transaction(*this); }

    bool PropertyGraphStore::check_node_exists_on_disk(size_t id) {
        nodes_file.clear();
        nodes_file.seekp(0, std::ios::cur); // Sincronizza lo stato dello stream misto
        nodes_file.seekg(0, std::ios::end);

        std::streampos file_size = nodes_file.tellg();
        if (file_size == std::streampos(-1) || (id * sizeof(NodeRecord)) >= static_cast<uint64_t>(file_size)) {
            return false;
        }

        nodes_file.seekg(id * sizeof(NodeRecord), std::ios::beg);
        NodeRecord rec;
        if (!nodes_file.read(reinterpret_cast<char*>(&rec), sizeof(NodeRecord))) {
            return false;
        }
        return rec.exists;
    }

    bool PropertyGraphStore::check_edge_exists_on_disk(size_t id) {
        edges_file.clear();
        edges_file.seekp(0, std::ios::cur); // Sincronizza lo stato dello stream misto
        edges_file.seekg(0, std::ios::end);
        std::streampos file_size = edges_file.tellg();
        if (file_size == std::streampos(-1) || (id * sizeof(EdgeRecord)) >= static_cast<uint64_t>(file_size)) {
            return false;
        }
        edges_file.seekg(id * sizeof(EdgeRecord), std::ios::beg);
        EdgeRecord rec;
        if (!edges_file.read(reinterpret_cast<char *>(&rec), sizeof(EdgeRecord))) return false;
        return rec.exists;
    }

    bool PropertyGraphStore::fetch_node_from_disk(size_t id, NodeData &data) {
        nodes_file.seekg(id * sizeof(NodeRecord), std::ios::beg);
        NodeRecord rec;
        if (!nodes_file.read(reinterpret_cast<char *>(&rec), sizeof(NodeRecord)) || !rec.exists) return false;
        data.id = id;
        if (rec.payload_size > 0 && rec.payload_offset != DISK_NULL) {
            std::string buf(rec.payload_size, '\0');
            payloads_file.seekg(rec.payload_offset, std::ios::beg);
            payloads_file.read(&buf[0], rec.payload_size);
            deserialize_payload(buf, data.labels, data.properties);
        }
        return true;
    }

    bool PropertyGraphStore::fetch_edge_from_disk(size_t id, EdgeData &data) {
        edges_file.seekg(id * sizeof(EdgeRecord), std::ios::beg);
        EdgeRecord rec;
        if (!edges_file.read(reinterpret_cast<char *>(&rec), sizeof(EdgeRecord)) || !rec.exists) return false;
        data.id = id;
        data.source_id = rec.source_id;
        data.target_id = rec.target_id;
        if (rec.payload_size > 0 && rec.payload_offset != DISK_NULL) {
            std::string buf(rec.payload_size, '\0');
            payloads_file.seekg(rec.payload_offset, std::ios::beg);
            payloads_file.read(&buf[0], rec.payload_size);
            deserialize_payload(buf, data.labels, data.properties);
        }
        return true;
    }

    std::string PropertyGraphStore::serialize_payload(const std::vector<std::string> &labels,
                                                      const std::unordered_map<std::string, PropertyValue> &props) {
        std::ostringstream ss(std::ios::binary);
        uint32_t label_count = static_cast<uint32_t>(labels.size());
        ss.write(reinterpret_cast<const char *>(&label_count), sizeof(label_count));
        for (const auto &l: labels) {
            uint16_t len = static_cast<uint16_t>(l.size());
            ss.write(reinterpret_cast<const char *>(&len), sizeof(len));
            ss.write(l.data(), len);
        }
        uint32_t prop_count = static_cast<uint32_t>(props.size());
        ss.write(reinterpret_cast<const char *>(&prop_count), sizeof(prop_count));
        for (const auto &[key, prop_val]: props) {
            uint16_t key_len = static_cast<uint16_t>(key.size());
            ss.write(reinterpret_cast<const char *>(&key_len), sizeof(key_len));
            ss.write(key.data(), key_len);
            uint8_t type_byte = static_cast<uint8_t>(prop_val.type);
            ss.write(reinterpret_cast<const char *>(&type_byte), sizeof(type_byte));
            if (prop_val.type == PropertyType::SPARSE_TENSOR) {
                const auto &tensor = std::get<SparseTensor>(prop_val.value);
                uint8_t dt = static_cast<uint8_t>(tensor.data_type);
                ss.write(reinterpret_cast<const char *>(&dt), sizeof(dt));
                ss.write(reinterpret_cast<const char *>(&tensor.rank), sizeof(tensor.rank));
                ss.write(reinterpret_cast<const char *>(tensor.shape.data()), tensor.rank * sizeof(uint64_t));
                if (tensor.data_type == TensorDataType::INT64) {
                    int64_t def = std::get<int64_t>(tensor.default_value);
                    ss.write(reinterpret_cast<const char *>(&def), sizeof(def));
                    uint64_t num_elements = tensor.coordinates.size();
                    ss.write(reinterpret_cast<const char *>(&num_elements), sizeof(num_elements));
                    for (const auto &coord: tensor.coordinates)
                        ss.write(reinterpret_cast<const char *>(coord.data()), tensor.rank * sizeof(uint64_t));
                    const auto &vals = std::get<std::vector<int64_t> >(tensor.values);
                    ss.write(reinterpret_cast<const char *>(vals.data()), num_elements * sizeof(int64_t));
                } else {
                    double def = std::get<double>(tensor.default_value);
                    ss.write(reinterpret_cast<const char *>(&def), sizeof(def));
                    uint64_t num_elements = tensor.coordinates.size();
                    ss.write(reinterpret_cast<const char *>(&num_elements), sizeof(num_elements));
                    for (const auto &coord: tensor.coordinates)
                        ss.write(reinterpret_cast<const char *>(coord.data()), tensor.rank * sizeof(uint64_t));
                    const auto &vals = std::get<std::vector<double>>(tensor.values);
                    ss.write(reinterpret_cast<const char *>(vals.data()), num_elements * sizeof(double));
                }
            } else if (prop_val.type == PropertyType::STRING) {
                const auto &str = std::get<std::string>(prop_val.value);
                uint32_t str_len = static_cast<uint32_t>(str.size());
                ss.write(reinterpret_cast<const char *>(&str_len), sizeof(str_len));
                ss.write(str.data(), str_len);
            } else if (prop_val.type == PropertyType::INT) {
                int64_t val = std::get<int64_t>(prop_val.value);
                ss.write(reinterpret_cast<const char *>(&val), sizeof(val));
            } else if (prop_val.type == PropertyType::DOUBLE) {
                double val = std::get<double>(prop_val.value);
                ss.write(reinterpret_cast<const char *>(&val), sizeof(val));
            } else if (prop_val.type == PropertyType::BOOL) {
                bool val = std::get<bool>(prop_val.value);
                ss.write(reinterpret_cast<const char *>(&val), sizeof(val));
            }
        }
        return ss.str();
    }

    void PropertyGraphStore::deserialize_payload(const std::string &buffer, std::vector<std::string> &labels,
                                                 std::unordered_map<std::string, PropertyValue> &props) {
        std::istringstream ss(buffer, std::ios::binary);
        uint32_t label_count = 0;
        ss.read(reinterpret_cast<char *>(&label_count), sizeof(label_count));
        labels.resize(label_count);
        for (uint32_t i = 0; i < label_count; ++i) {
            uint16_t len = 0;
            ss.read(reinterpret_cast<char *>(&len), sizeof(len));
            labels[i].resize(len);
            ss.read(&labels[i][0], len);
        }
        uint32_t prop_count = 0;
        ss.read(reinterpret_cast<char *>(&prop_count), sizeof(prop_count));
        for (uint32_t i = 0; i < prop_count; ++i) {
            uint16_t key_len = 0;
            ss.read(reinterpret_cast<char *>(&key_len), sizeof(key_len));
            std::string key(key_len, '\0');
            ss.read(&key[0], key_len);
            uint8_t type_byte = 0;
            ss.read(reinterpret_cast<char *>(&type_byte), sizeof(type_byte));
            PropertyType type = static_cast<PropertyType>(type_byte);
            PropertyValue prop_val;
            prop_val.type = type;
            if (type == PropertyType::SPARSE_TENSOR) {
                SparseTensor tensor;
                uint8_t dt = 0;
                ss.read(reinterpret_cast<char *>(&dt), sizeof(dt));
                tensor.data_type = static_cast<TensorDataType>(dt);
                ss.read(reinterpret_cast<char *>(&tensor.rank), sizeof(tensor.rank));
                tensor.shape.resize(tensor.rank);
                ss.read(reinterpret_cast<char *>(tensor.shape.data()), tensor.rank * sizeof(uint64_t));
                if (tensor.data_type == TensorDataType::INT64) {
                    int64_t def;
                    ss.read(reinterpret_cast<char *>(&def), sizeof(def));
                    tensor.default_value = def;
                    uint64_t num_elements = 0;
                    ss.read(reinterpret_cast<char *>(&num_elements), sizeof(num_elements));
                    tensor.coordinates.resize(num_elements, std::vector<uint64_t>(tensor.rank));
                    for (uint64_t n = 0; n < num_elements; ++n)
                        ss.read(reinterpret_cast<char *>(tensor.coordinates[n].data()), tensor.rank * sizeof(uint64_t));
                    std::vector<int64_t> vals(num_elements);
                    ss.read(reinterpret_cast<char *>(vals.data()), num_elements * sizeof(int64_t));
                    tensor.values = vals;
                } else {
                    double def;
                    ss.read(reinterpret_cast<char *>(&def), sizeof(def));
                    tensor.default_value = def;
                    uint64_t num_elements = 0;
                    ss.read(reinterpret_cast<char *>(&num_elements), sizeof(num_elements));
                    tensor.coordinates.resize(num_elements, std::vector<uint64_t>(tensor.rank));
                    for (uint64_t n = 0; n < num_elements; ++n)
                        ss.read(reinterpret_cast<char *>(tensor.coordinates[n].data()), tensor.rank * sizeof(uint64_t));
                    std::vector<double> vals(num_elements);
                    ss.read(reinterpret_cast<char *>(vals.data()), num_elements * sizeof(double));
                    tensor.values = vals;
                }
                prop_val.value = tensor;
            } else if (type == PropertyType::STRING) {
                uint32_t str_len = 0;
                ss.read(reinterpret_cast<char *>(&str_len), sizeof(str_len));
                std::string str(str_len, '\0');
                ss.read(&str[0], str_len);
                prop_val.value = str;
            } else if (type == PropertyType::INT) {
                int64_t val;
                ss.read(reinterpret_cast<char *>(&val), sizeof(val));
                prop_val.value = val;
            } else if (type == PropertyType::DOUBLE) {
                double val;
                ss.read(reinterpret_cast<char *>(&val), sizeof(val));
                prop_val.value = val;
            } else if (type == PropertyType::BOOL) {
                bool val;
                ss.read(reinterpret_cast<char *>(&val), sizeof(val));
                prop_val.value = val;
            }
            props[key] = prop_val;
        }
    }

    size_t PropertyGraphStore::get_next_node_id_from_disk() {
        nodes_file.clear();                  // Resetta qualsiasi flag di errore precedente
        nodes_file.seekp(0, std::ios::cur);  // Sincronizzazione cruciale Scrittura -> Lettura
        nodes_file.seekg(0, std::ios::end);  // Sposta in sicurezza il puntatore di lettura alla fine

        std::streampos file_size = nodes_file.tellg();
        if (file_size == std::streampos(-1)) {
            return 0; // Guardrail difensivo: se fallisce, il file è interpretabile come vuoto (ID 0)
        }
        return static_cast<size_t>(file_size / sizeof(NodeRecord));
    }

    size_t PropertyGraphStore::get_next_edge_id_from_disk() {
        edges_file.clear();                  // Resetta i flag
        edges_file.seekp(0, std::ios::cur);  // Sincronizzazione Scrittura -> Lettura
        edges_file.seekg(0, std::ios::end);  // Sposta alla fine

        std::streampos file_size = edges_file.tellg();
        if (file_size == std::streampos(-1)) {
            return 0;
        }
        return static_cast<size_t>(file_size / sizeof(EdgeRecord));
    }
}
