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

#include "logds/paramoneur/types/SparseTensor.h"


namespace logds::pg {


    constexpr  uint64_t DISK_NULL = 0xFFFFFFFFFFFFFFFF;

    enum class PropertyType : uint8_t {
        INT = 1,
        DOUBLE = 2,
        STRING = 3,
        BOOL = 4,
        SPARSE_TENSOR = 5
    };

    struct PropertyValue {
        PropertyType type;
        std::variant<int64_t, double, std::string, bool, types::SparseTensor> value;
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
