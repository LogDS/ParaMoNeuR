
//
// Created by gyankos on 22/10/26.
//

#ifndef PARAMONEUR2_ALIASES_H
#define PARAMONEUR2_ALIASES_H
#include <cstdint>
#include <string>
#include <variant>

namespace logds::paramoneur {
    // ---------------------------------------------------------------------------
    // Type aliases and small helpers
    // ---------------------------------------------------------------------------
    using FlatIndex = std::int64_t;
    using VarOrConst = std::variant<int64_t, std::string>;
    using ConstraintTriplet = std::tuple<VarOrConst, VarOrConst, bool>;
}

#endif //PARAMONEUR2_ALIASES_H
