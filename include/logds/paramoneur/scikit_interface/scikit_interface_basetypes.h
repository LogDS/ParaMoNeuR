//
// Created by gyankos on 23/10/26.
//

#ifndef PARAMONEUR2_SCIKIT_INTERFACE_BASETYPES_H
#define PARAMONEUR2_SCIKIT_INTERFACE_BASETYPES_H
#include <utility>
#include <variant>

namespace logds::paramoneur::scikit_interface {
    enum class Type {
        Interval,
        Proba,
        Class
    };


    enum class OutcomeClass {
        True,
        False,
        Unknown,
        Inconsistent,
        Other
    };

    using PotentialResult = std::variant<std::pair<double, double>, double, OutcomeClass>;

}

#endif //PARAMONEUR2_SCIKIT_INTERFACE_BASETYPES_H
