//
// Created by gyankos on 22/10/26.
//

#ifndef PARAMONEUR2_UNIFICATION_H
#define PARAMONEUR2_UNIFICATION_H
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "logds/paramoneur/language/compiled_structures.h"

namespace logds::paramoneur {
    class PureOneWayUnifier {
        // Directional transformation maps, isolated from each other
        std::unordered_map<std::string, logds::paramoneur::Argument> mapLeftToRight;
        std::unordered_map<std::string, logds::paramoneur::Argument> mapRightToLeft;
    public:
        PureOneWayUnifier reverse() const;
        static PureOneWayUnifier compose(const PureOneWayUnifier& lhs, const PureOneWayUnifier& rhs);
        logds::paramoneur::Argument resolveLeftVariable(const std::string& varName) const;
        logds::paramoneur::Argument resolveRightVariable(const std::string& varName) const;
        bool evaluateLeftToRight(const logds::paramoneur::Argument& argLeft, const logds::paramoneur::Argument& argRight);
        bool evaluateRightToLeft(const logds::paramoneur::Argument& argLeft, const logds::paramoneur::Argument& argRight);
    };


// def are_rules_equivalent_modulo_unification(rule_1, rule_2, doAlpha = True) -> bool
bool areRulesEquivalentModuloUnification(const UltimateHornRule& rule1,
                                         const UltimateHornRule& rule2,
                                         bool doAlpha = true);


    // def have_rules_same_body_and_compatible_head(rule_1, rule_2, strictHeadEvaluation=False)
// Returns std::nullopt where the Python code returns None.
std::optional<PureOneWayUnifier> haveRulesSameBodyAndCompatibleHead(const UltimateHornRule& rule1,
                                                                    const UltimateHornRule& rule2,
                                                                    bool strictHeadEvaluation = false);


std::vector<std::pair<std::vector<int>, PureOneWayUnifier>>
computeClauseEquivalenceClassesTransitiveClosure(const std::vector<UltimateHornRule>& clausesList);
}




#endif //PARAMONEUR2_UNIFICATION_H
