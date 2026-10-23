//
// Created by gyankos on 22/10/26.
//

#include "logds/paramoneur/language/parsed_structures.h"

#include <sstream>
#include <ranges>
#include <yaucl/strings/string_utils.h>

std::ostream & logds::paramoneur::operator<<(std::ostream &os, const StrArgument &obj) {
    return os << obj.name;
}

std::ostream & logds::paramoneur::operator<<(std::ostream &os, const StrAtom &obj) {
    const std::string neg = obj.isNegated ? std::string(kNegationSymbol) : std::string();
    std::string argText;
    if (!obj.args.empty()) {
        argText = yaucl::strings::join(obj.args, ", ", [](const auto& arg) {
            std::stringstream ss;
            ss << arg;
            return ss.str();
        }, "(", ")");
    }
    return os << neg + obj.predicate + argText;
}
