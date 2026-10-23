//
// Created by gyankos on 22/10/26.
//

#include "logds/paramoneur/environment/GlobalEnvironmentProxy.h"

#include "logds/paramoneur/algorithms/indexing.h"

namespace logds::paramoneur {
    std::optional<Argument> GlobalEnvironmentProxy::atomFromName(const std::string &id,
                                                                 bool isVarConstantOtherwise) const {
        if (!isVarConstantOtherwise) {
            auto it = constant_strs.find(id);
            auto it2 = constants_and_var_map.find(id);
            if (it != constant_strs.end() && it2 != constants_and_var_map.end()) {
                return Argument{ id,it2->second, ArgType::Constant};
            }
        } else {
            auto it = vars_strs.find(id);
            auto it2 = constants_and_var_map.find(id);
            if (it != vars_strs.end() && it2 != constants_and_var_map.end()) {
                return Argument{ id,it2->second, ArgType::Variable};
            }
        }
        return std::nullopt;
    }

    std::optional<Argument> GlobalEnvironmentProxy::atomFromId(const int id, bool isVarConstantOtherwise) const {
        if (!isVarConstantOtherwise) {
            auto it = constant_idx.find(id);
            auto it2 = constants_and_var_map_inv.find(id);
            if (it != constant_idx.end() && it2 != constants_and_var_map_inv.end()) {
                return Argument{ it2->second,id, ArgType::Constant};
            }
        } else {
            auto it = vars_idx.find(id);
            auto it2 = constants_and_var_map_inv.find(id);
            if (it != vars_idx.end() && it2 != constants_and_var_map_inv.end()) {
                return Argument{ it2->second,id, ArgType::Variable};
            }
        }
        return std::nullopt;
    }

    EqualityConstraint GlobalEnvironmentProxy::equalityConstraintFromTriplet(const ConstraintTriplet &triplet) const {
        const auto& [a, b, c] = triplet;
        std::optional<Argument> argA, argB;
        if (std::holds_alternative<std::string>(a)) {
            argA = atomFromName(std::get<std::string>(a), true);
        } else {
            argA = atomFromId(std::get<int>(a), false);
        }
        if (std::holds_alternative<std::string>(b)) {
            argB = atomFromName(std::get<std::string>(b), true);
        } else {
            argB = atomFromId(std::get<int>(b), false);
        }
        assert(argA && argB);
        return EqualityConstraint{argA.value(), argB.value(), c};
    }

    uint64_t GlobalEnvironmentProxy::compileAtomToIndex(const logds::paramoneur::ArbitraryAtom &g_atom) {
        return compile_atom_to_index_param(g_atom, max_B(), max_L(), kStopToken);
    }

    Argument GlobalEnvironmentProxy::
    instantiateArgument(const Argument &ref, const std::unordered_map<int, int> &pairs) {
        if (ref.type == ArgType::Variable) {
            auto it = pairs.find(ref.id);
            if (it != pairs.end()) {
                auto const_id = it->second;
                auto it2 = constants_and_var_map_inv.find(const_id);
                if (it2 != constants_and_var_map_inv.end()) {
                    return {ArgType::Constant, it->second, it2->second};
                }
            }
        }
        return ref;
    }

    Argument GlobalEnvironmentProxy::
    instantiateArgument(const Argument &ref, const std::unordered_map<int, std::string> &pairs) {
        if (ref.type == ArgType::Variable) {
            auto it = pairs.find(ref.id);
            if (it != pairs.end()) {
                const auto& const_id = it->second;
                auto it2 = constants_and_var_map.find(const_id);
                if (it2 != constants_and_var_map.end()) {
                    return {ArgType::Constant, it2->second, const_id};
                }
            }
        }
        return ref;
    }

    Argument GlobalEnvironmentProxy::
instantiateArgument(const Argument &ref, const std::unordered_map<std::string, int> &pairs) {
        if (ref.type == ArgType::Variable) {
            auto it = pairs.find(ref.name);
            if (it != pairs.end()) {
                const auto& const_id = it->second;
                auto it2 = constants_and_var_map_inv.find(const_id);
                if (it2 != constants_and_var_map_inv.end()) {
                    return {ArgType::Constant, const_id, it2->second};
                }
            }
        }
        return ref;
    }

    ArbitraryAtom GlobalEnvironmentProxy::instantiateAtom(const ArbitraryAtom &head,
                                                          const std::unordered_map<int, int> &pairs) {
        if (head.predicate == kPredicateFalsehood || (head.name == kBotSymbol))
            return head;
        std::vector<Argument> bodyLiterals;
        for (const auto& ref : head.args)
            bodyLiterals.emplace_back(instantiateArgument(ref, pairs));
        return {head.sign, head.predicate, bodyLiterals, head.name};
    }

    BodyLiteral GlobalEnvironmentProxy::instantiateBodyLiteral(const BodyLiteral &ref,
        const std::unordered_map<int, int> &pairs) {
        if (ref.atom) {
            return {instantiateAtom(*ref.atom, pairs), ref.isWeaklyNegated};
        } else {
            return {ref.isWeaklyNegated};
        }
    }

    EqualityConstraint GlobalEnvironmentProxy::instantiateConstraint(const EqualityConstraint &ref,
        const std::unordered_map<int, int> &pairs) {
        return {instantiateArgument(ref.left, pairs), instantiateArgument(ref.right, pairs), ref.isEqual};
    }

    UltimateHornRule GlobalEnvironmentProxy::instantiateRule(const UltimateHornRule &rule,
                                                             const std::unordered_map<int, int> &pairs) {
        auto head = instantiateAtom(rule.head, pairs);
        std::vector<BodyLiteral> bodyLiterals;
        std::vector<EqualityConstraint> equalityConstraints;
        for (const auto& ref : rule.tail)
            bodyLiterals.push_back(instantiateBodyLiteral(ref, pairs));
        for (const auto& ref : rule.constraints)
            equalityConstraints.push_back(instantiateConstraint(ref, pairs));
        return {head, bodyLiterals, equalityConstraints, rule.isIntegrityConstraint, rule.weight};
    }
}
