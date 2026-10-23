//
// Created by gyankos on 23/10/26.
//

#ifndef PARAMONEUR2_PARACONSISTENTTHEORYCONTRACTION_H
#define PARAMONEUR2_PARACONSISTENTTHEORYCONTRACTION_H
#include <set>
#include <vector>

#include "logds/paramoneur/language/compiled_structures.h"

namespace logds::paramoneur {
    class GlobalEnvironmentProxy;

    class PruneRulesWhileCollectinMISes {
public:
    std::set<std::vector<int>> MIS;
    GlobalEnvironmentProxy* env;

    PruneRulesWhileCollectinMISes();

    // Python: _expand_MIS_over_ground_rule
    bool expandMISOverGroundRule(const UltimateHornRule& r2);

    // Python: removeRulesHavingMisInBody
    std::shared_ptr<UltimateHornRule> removeRulesHavingMisInBody(const UltimateHornRule& rule);

    // Python: retainRuleAsNonBot
    /*
     * :param rule:
     * :param collection_of_mis:
     * :return: False if this triggered some insertion in collection_of_mis, as the rule needs to be
     *          filtered out, and true otherwise.
     *
     * (Original docstring, translated: parameters as above; returns false if this triggered some
     *  insertion in collection_of_mis, so the rule must be filtered out, and true otherwise.)
     */
    bool retainRuleAsNonBot(const UltimateHornRule& rule);
    };


// Python: is_rule_tail_inconsistent_via_mis
/*
 * Verifica se la coda di una UltimateHornRule contiene un sottoinsieme di atomi compatibile con un iper-arco MIS
 * oppure se contiene un'autocontraddizione intrinseca strutturale basata su Strong Negation (esistenza simultanea di A e ¬A).
 * Sfrutta map e filter in programmazione funzionale lazy per massimizzare le performance a basso livello senza errori di scope.
 *
 * (Original docstring, translated: checks whether the tail of a UltimateHornRule contains a subset of atoms
 *  compatible with a MIS hyperedge, or a structural intrinsic self-contradiction based on Strong Negation
 *  (simultaneous existence of A and ¬A). It uses lazy map and filter functional programming to maximize
 *  low-level performance without scope errors.)
 */
bool isRuleTailInconsistentViaMis(const UltimateHornRule& rule,
                                  const std::vector<std::vector<int>>& collectionOfMis,
                                  int bVal,
                                  int maxLParam,
                                  int stopToken);

// Python: execute_paraconsistent_theory_contraction
/* Scansiona l'intero lotto di regole canoniche espellendo quelle strutturalmente vulnerabili all'incoerenza o autocontraddittorie.
 * (Scans the whole batch of canonical rules, expelling those structurally vulnerable to inconsistency or self-contradiction.)
 */
std::vector<UltimateHornRule> executeParaconsistentTheoryContraction(
    const std::vector<UltimateHornRule>& coreRulesList,
    const std::vector<std::vector<int>>& collectionOfMis,
    int bVal,
    int maxLParam,
    int stopToken);
}

#endif //PARAMONEUR2_PARACONSISTENTTHEORYCONTRACTION_H
