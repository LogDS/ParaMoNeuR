//
// Created by gyankos on 22/10/26.
//

#include "yaucl/structures/map_operations.h"
#include <logds/paramoneur/algorithms/unification.h>

#include "logds/paramoneur/algorithms/canonical_form.h"


logds::paramoneur::PureOneWayUnifier logds::paramoneur::PureOneWayUnifier::reverse() const {
    PureOneWayUnifier result;
    result.mapLeftToRight = mapRightToLeft;
    result.mapRightToLeft = mapLeftToRight;
    return result;
}

logds::paramoneur::PureOneWayUnifier logds::paramoneur::PureOneWayUnifier::compose(const PureOneWayUnifier &lhs, const PureOneWayUnifier &rhs) {
    PureOneWayUnifier result;
    result.mapLeftToRight = composeMap(lhs.mapLeftToRight, rhs.mapLeftToRight);
    result.mapRightToLeft = composeMap(rhs.mapRightToLeft, lhs.mapRightToLeft);
    return result;
}

logds::paramoneur::Argument logds::paramoneur::PureOneWayUnifier::resolveLeftVariable(const std::string &varName) const {
    std::string current = varName;
    std::unordered_set<std::string> seen{current};
    while (true) {
        auto it = mapLeftToRight.find(current);
        if (it == mapLeftToRight.end()) {
            break;
        }
        const logds::paramoneur::Argument target = it->second;
        if (target.type != logds::paramoneur::ArgType::Variable) {
            return target;
        }
        std::string nextName = target.name;  // str(target.name)
        if (seen.find(nextName) != seen.end() || nextName == current) {
            break;
        }
        seen.insert(nextName);
        current = nextName;
    }
    return logds::paramoneur::Argument{logds::paramoneur::ArgType::Variable, 0, current};
}

logds::paramoneur::Argument logds::paramoneur::PureOneWayUnifier::resolveRightVariable(const std::string &varName) const {
    std::string current = varName;
    std::unordered_set<std::string> seen{current};
    while (true) {
        auto it = mapRightToLeft.find(current);
        if (it == mapRightToLeft.end()) {
            break;
        }
        const logds::paramoneur::Argument target = it->second;
        if (target.type != logds::paramoneur::ArgType::Variable) {
            return target;
        }
        std::string nextName = target.name;  // str(target.name)
        if (seen.find(nextName) != seen.end() || nextName == current) {
            break;
        }
        seen.insert(nextName);
        current = nextName;
    }
    return logds::paramoneur::Argument{logds::paramoneur::ArgType::Variable, 0, current};
}

bool logds::paramoneur::PureOneWayUnifier::evaluateLeftToRight(const logds::paramoneur::Argument &argLeft,
                                                               const logds::paramoneur::Argument &argRight) {
    if (argLeft.type == logds::paramoneur::ArgType::Constant && argRight.type == logds::paramoneur::ArgType::Constant) {
        return argLeft.name == argRight.name;
    }

    if (argLeft.type == logds::paramoneur::ArgType::Variable) {
        if (mapLeftToRight.find(argLeft.name) == mapLeftToRight.end()) {
            mapLeftToRight.emplace(argLeft.name, argRight);
        }
        logds::paramoneur::Argument resolvedL = resolveLeftVariable(argLeft.name);
        if (resolvedL.type == logds::paramoneur::ArgType::Constant) {
            return argRight.type == logds::paramoneur::ArgType::Constant && resolvedL.name == argRight.name;
        }

        std::string nameLRes = resolvedL.name;
        return nameLRes == argRight.name && resolvedL.type == argRight.type;
    }

    return false;
}

bool logds::paramoneur::PureOneWayUnifier::evaluateRightToLeft(const logds::paramoneur::Argument &argLeft,
                                                               const logds::paramoneur::Argument &argRight) {
    if (argLeft.type == logds::paramoneur::ArgType::Constant && argRight.type == logds::paramoneur::ArgType::Constant) {
        return argLeft.name == argRight.name;
    }

    if (argRight.type == ArgType::Variable) {
        if (mapRightToLeft.find(argRight.name) == mapRightToLeft.end()) {
            mapRightToLeft.emplace(argRight.name, argLeft);
        }
        logds::paramoneur::Argument resolvedR = resolveRightVariable(argRight.name);
        if (resolvedR.type == logds::paramoneur::ArgType::Constant) {
            return argLeft.type == logds::paramoneur::ArgType::Constant && resolvedR.name == argLeft.name;
        }

        std::string nameRRes = resolvedR.name;
        // if name_r_res in self.map_right_to_left:
        return nameRRes == argLeft.name && resolvedR.type == argLeft.type;

        // self.map_right_to_left[name_r_res] = arg_left
        // return True
    }

    return false;
}

bool logds::paramoneur::areRulesEquivalentModuloUnification(const UltimateHornRule &rule1,
    const UltimateHornRule &rule2, bool doAlpha) {
    const UltimateHornRule cRule1 = doAlpha ? getCanonicalAlphaForm(rule1) : rule1;
    const UltimateHornRule cRule2 = doAlpha ? getCanonicalAlphaForm(rule2) : rule2;

    if (cRule1.isIntegrityConstraint != cRule2.isIntegrityConstraint) return false;
    if (cRule1.tail.size() != cRule2.tail.size() || cRule1.constraints.size() != cRule2.constraints.size()) return false;

    PureOneWayUnifier unifier;

    if (!cRule1.isIntegrityConstraint) {
        if (cRule1.head.sign != cRule2.head.sign || cRule1.head.predicate != cRule2.head.predicate) return false;
        const std::size_t headLen = std::min(cRule1.head.args.size(), cRule2.head.args.size());  // zip
        for (std::size_t k = 0; k < headLen; ++k) {
            const Argument& al = cRule1.head.args[k];
            const Argument& ar = cRule2.head.args[k];
            if (!unifier.evaluateLeftToRight(al, ar) || !unifier.evaluateRightToLeft(al, ar)) return false;
        }
    }

    const std::size_t tailLen = std::min(cRule1.tail.size(), cRule2.tail.size());  // zip
    for (std::size_t t = 0; t < tailLen; ++t) {
        const auto& lit1 = cRule1.tail[t];
        const auto& lit2 = cRule2.tail[t];
        if (lit1.isWeaklyNegated != lit2.isWeaklyNegated) return false;
        if (lit1.atom->sign != lit2.atom->sign || lit1.atom->predicate != lit2.atom->predicate) return false;
        const std::size_t argLen = std::min(lit1.atom->args.size(), lit2.atom->args.size());  // zip
        for (std::size_t k = 0; k < argLen; ++k) {
            const Argument& al = lit1.atom->args[k];
            const Argument& ar = lit2.atom->args[k];
            if (!unifier.evaluateLeftToRight(al, ar) || !unifier.evaluateRightToLeft(al, ar)) return false;
        }
    }

    const std::size_t constLen = std::min(cRule1.constraints.size(), cRule2.constraints.size());  // zip
    for (std::size_t c = 0; c < constLen; ++c) {
        const auto& const1 = cRule1.constraints[c];
        const auto& const2 = cRule2.constraints[c];
        if (const1.isEqual != const2.isEqual) return false;
        if (!unifier.evaluateLeftToRight(const1.left, const2.left) || !unifier.evaluateRightToLeft(
                const1.left, const2.left)) return false;
        if (!unifier.evaluateLeftToRight(const1.right, const2.right) || !unifier.evaluateRightToLeft(
                const1.right, const2.right)) return false;
    }

    return true;
}

std::optional<logds::paramoneur::PureOneWayUnifier> logds::paramoneur::haveRulesSameBodyAndCompatibleHead(
    const UltimateHornRule &rule1, const UltimateHornRule &rule2, bool strictHeadEvaluation) {
    const UltimateHornRule cRule1 = getCanonicalAlphaForm(rule1);
    const UltimateHornRule cRule2 = getCanonicalAlphaForm(rule2);

    if (cRule1.tail.size() != cRule2.tail.size() || cRule1.constraints.size() != cRule2.constraints.size()) {
        return std::nullopt;
    }

    PureOneWayUnifier unifier;

    const std::size_t tailLen = std::min(cRule1.tail.size(), cRule2.tail.size());  // zip
    for (std::size_t t = 0; t < tailLen; ++t) {
        const auto& lit1 = cRule1.tail[t];
        const auto& lit2 = cRule2.tail[t];
        if (lit1.isWeaklyNegated != lit2.isWeaklyNegated) return std::nullopt;
        if (lit1.atom->sign != lit2.atom->sign || lit1.atom->predicate != lit2.atom->predicate) return std::nullopt;
        const std::size_t argLen = std::min(lit1.atom->args.size(), lit2.atom->args.size());  // zip
        for (std::size_t k = 0; k < argLen; ++k) {
            const Argument& al = lit1.atom->args[k];
            const Argument& ar = lit2.atom->args[k];
            if (!unifier.evaluateLeftToRight(al, ar) || !unifier.evaluateRightToLeft(al, ar)) return std::nullopt;
        }
    }

    const std::size_t constLen = std::min(cRule1.constraints.size(), cRule2.constraints.size());  // zip
    for (std::size_t c = 0; c < constLen; ++c) {
        const auto& const1 = cRule1.constraints[c];
        const auto& const2 = cRule2.constraints[c];
        if (const1.isEqual != const2.isEqual) return std::nullopt;
        if (!unifier.evaluateLeftToRight(const1.left, const2.left) || !unifier.evaluateRightToLeft(
                const1.left, const2.left)) return std::nullopt;
        if (!unifier.evaluateLeftToRight(const1.right, const2.right) || !unifier.evaluateRightToLeft(
                const1.right, const2.right)) return std::nullopt;
    }

    if (cRule1.isIntegrityConstraint != cRule2.isIntegrityConstraint) return std::nullopt;
    if (cRule1.isIntegrityConstraint) return unifier;

    if (cRule1.head.sign != cRule2.head.sign || cRule1.head.predicate != cRule2.head.predicate) {
        if (strictHeadEvaluation) {
            return std::nullopt;
        } else {
            return unifier;
        }
    }

    const std::size_t headLen = std::min(cRule1.head.args.size(), cRule2.head.args.size());  // zip
    for (std::size_t k = 0; k < headLen; ++k) {
        const Argument& al = cRule1.head.args[k];
        const Argument& ar = cRule2.head.args[k];
        if (!unifier.evaluateLeftToRight(al, ar) || !unifier.evaluateRightToLeft(al, ar)) return std::nullopt;
    }

    return unifier;
}

std::vector<std::pair<std::vector<int>, logds::paramoneur::PureOneWayUnifier>> logds::paramoneur::
computeClauseEquivalenceClassesTransitiveClosure(const std::vector<UltimateHornRule> &clausesList) {

    // def compute_clause_equivalence_classes_transitive_closure(clauses_list)
    using UnifierChain = std::pair<std::vector<int>, PureOneWayUnifier>;
    std::vector<UnifierChain> activeChainsPool;
    const std::size_t numClauses = clausesList.size();

    for (std::size_t i = 0; i < numClauses; ++i) {
        for (std::size_t j = 0; j < numClauses; ++j) {
            if (j <= i) continue;
            std::optional<PureOneWayUnifier> unifierInstance =
                    haveRulesSameBodyAndCompatibleHead(clausesList[i], clausesList[j]);
            if (unifierInstance.has_value()) {
                activeChainsPool.push_back({std::vector<int>{static_cast<int>(i), static_cast<int>(j)},
                    *unifierInstance});
            }
        }
    }

    bool stabilized = false;
    while (!stabilized) {
        stabilized = true;
        std::unordered_set<std::size_t> mergedThisRound;
        std::vector<UnifierChain> newChainsGenerated;

        // itertools.combinations(range(len(active_chains_pool)), 2), with the length
        // captured once at the start of the round as in Python
        const std::size_t poolSize = activeChainsPool.size();
        for (std::size_t leftIdx = 0; leftIdx < poolSize && stabilized; ++leftIdx) {
            for (std::size_t rightIdx = leftIdx + 1; rightIdx < poolSize && stabilized; ++rightIdx) {
                if (mergedThisRound.find(leftIdx) != mergedThisRound.end() ||
                    mergedThisRound.find(rightIdx) != mergedThisRound.end()) continue;

                std::vector<int> tupleL = activeChainsPool[leftIdx].first;
                PureOneWayUnifier unifL = activeChainsPool[leftIdx].second;
                std::vector<int> tupleR = activeChainsPool[rightIdx].first;
                PureOneWayUnifier unifR = activeChainsPool[rightIdx].second;

                if (tupleL.back() == tupleR.back()) {
                    std::reverse(tupleR.begin(), tupleR.end());  // tuple(reversed(tuple_r))
                    unifR = unifR.reverse();
                } else if (tupleL.front() == tupleR.front()) {
                    std::reverse(tupleL.begin(), tupleL.end());  // tuple(reversed(tuple_l))
                    unifL = unifL.reverse();
                }
                if (tupleL.back() == tupleR.front()) {
                    PureOneWayUnifier composedUnifier = PureOneWayUnifier::compose(unifL, unifR);

                    std::size_t firstClauseIdx = static_cast<std::size_t>(tupleL.front());
                    std::size_t lastClauseIdx = static_cast<std::size_t>(tupleR.back());
                    std::optional<PureOneWayUnifier> unifierInstance =
                            haveRulesSameBodyAndCompatibleHead(clausesList[firstClauseIdx], clausesList[lastClauseIdx]);
                    if (unifierInstance.has_value()) {
                        std::vector<int> combinedTuple = tupleL;  // tuple_l + tuple_r[1:]
                        combinedTuple.insert(combinedTuple.end(), tupleR.begin() + 1, tupleR.end());
                        newChainsGenerated.push_back({combinedTuple, composedUnifier});

                        // [x for idx, x in enumerate(pool) if idx not in (left, right)]
                        std::vector<UnifierChain> filtered;
                        for (std::size_t idx = 0; idx < activeChainsPool.size(); ++idx) {
                            if (idx != leftIdx && idx != rightIdx) {
                                filtered.push_back(activeChainsPool[idx]);
                            }
                        }
                        activeChainsPool = std::move(filtered);
                        stabilized = false;
                        // Immediate break to safely rebuild the atomic state of the pool
                        break;
                    }
                }
            }
        }

        if (!stabilized) {
            activeChainsPool.insert(activeChainsPool.end(), newChainsGenerated.begin(), newChainsGenerated.end());
        }
    }

    // [(tuple(sorted(tuple(set(chain)))), unif) for chain, unif in active_chains_pool]
    std::vector<std::pair<std::vector<int>, PureOneWayUnifier>> result;
    result.reserve(activeChainsPool.size());
    for (const auto& [chain, unif] : activeChainsPool) {
        std::unordered_set<int> uniqueElements(chain.begin(), chain.end());
        std::vector<int> sortedChain(uniqueElements.begin(), uniqueElements.end());
        std::sort(sortedChain.begin(), sortedChain.end());
        result.push_back({sortedChain, unif});
    }
    return result;
}
