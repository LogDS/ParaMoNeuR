//
// Created by gyankos on 23/10/26.
//

#include <iostream>
#include <logds/paramoneur/algorithms/ParaconsistentTheoryContraction.h>

#include "logds/paramoneur/algorithms/indexing.h"
#include "logds/paramoneur/environment/GlobalEnvironmentProxy.h"
#include "yaucl/structures/map_operations.h"
#include "yaucl/structures/setoids/basics.h"

// Python: def areGroundedConstraintsSatisfied(r2)
bool areGroundedConstraintsSatisfied(const logds::paramoneur::UltimateHornRule& r2) {
    bool skip = false;
    for (const auto& constraint : r2.constraints) {
        // Se il vincolo richiede eguaglianza (X = Y) ma i valori differiscono -> Invalido
        // (If the constraint requires equality (X = Y) but the values differ -> Invalid)
        if (constraint.isEqual && constraint.left.id != constraint.right.id) {
            skip = true;
            break;
        }
        // Se il vincolo richiede diseguaglianza (X != Y) ma i valori coincidono -> Invalido
        // (If the constraint requires inequality (X != Y) but the values coincide -> Invalid)
        if (!constraint.isEqual && constraint.left.id == constraint.right.id) {
            skip = true;
            break;
        }
    }
    return !skip;
}

logds::paramoneur::PruneRulesWhileCollectinMISes::PruneRulesWhileCollectinMISes(): MIS(), env(&GlobalEnvironmentProxy::getInstance()) {}

bool logds::paramoneur::PruneRulesWhileCollectinMISes::expandMISOverGroundRule(const UltimateHornRule &r2) {
    bool continua = areGroundedConstraintsSatisfied(r2);
    std::vector<int64_t> L;
    if (continua) {
        for (const auto& lit : r2.tail) {
            if (!lit.isWeaklyNegated && lit.atom) {
                L.push_back(env->compileAtomToIndex(*lit.atom));
            }
        }
    } else {
        return false;
    }
    if (!L.empty()) {
        MIS.insert(L);
        return false;
    } else {
        return true;
    }
}

std::shared_ptr<logds::paramoneur::UltimateHornRule> logds::paramoneur::PruneRulesWhileCollectinMISes::
removeRulesHavingMisInBody(const UltimateHornRule &rule) {
    auto FV = rule.fvName();
    if (!FV.empty()) {
        std::vector<int64_t> lFV(FV.begin(), FV.end());
        std::vector<int64_t> constants{env->constant_idx.begin(), env->constant_idx.end()};
        int ruleHavingMIS = 0;
        int countConsidered = 0;
        std::unordered_map<int, std::set<int>> L;

        forEachTuple<int64_t>(constants, lFV.size(), [&](const std::vector<int64_t>& x) {
            std::unordered_map<int64_t, int64_t> D = zipToMap(lFV, x);
            UltimateHornRule r2 = env->instantiateRule(rule, D);
            if (areGroundedConstraintsSatisfied(r2)) {
                ++countConsidered;
                std::set<int64_t> S;
                for (const auto& lit : r2.tail) {
                    if (!lit.isWeaklyNegated && lit.atom) {
                        S.insert(env->compileAtomToIndex(*lit.atom));
                    }
                }
                for (const auto& mis : MIS) {
                    if (supersetOf(mis, S)) {
                        ++ruleHavingMIS;
                        // for k, v in D.items(): L[k].add(v)  (D items in lFV order)
                        for (std::size_t i = 0; i < lFV.size(); ++i) {
                            L[lFV[i]].insert(x[i]);
                        }
                        break;
                    }
                }
            }
            return false;
        });

        if (ruleHavingMIS == countConsidered) {
            return nullptr;
        } else {
            std::vector<EqualityConstraint> Ll = rule.constraints;
            for (const auto& [varIdx, entrySecond] : L) {
                const auto forneq = env->atomFromId(varIdx, true);
                for (const auto& x : entrySecond) {
                    Ll.push_back(EqualityConstraint(*forneq, *env->atomFromId(x, false), false));
                }
            }
            return std::make_shared<UltimateHornRule>(rule.head, rule.tail, Ll,
                                                      rule.isIntegrityConstraint, rule.weight);
        }
    } else {
        const UltimateHornRule& r2 = rule;
        if (areGroundedConstraintsSatisfied(r2)) {
            std::set<int64_t> L;
            for (const auto& lit : r2.tail) {
                if (!lit.isWeaklyNegated && lit.atom) {
                    L.insert(env->compileAtomToIndex(*lit.atom));
                }
            }
            for (const auto& mis : MIS) {
                if (supersetOf(mis, L)) {
                    return nullptr;
                }
            }
            return std::make_shared<UltimateHornRule>(rule);
        } else {
            return nullptr;
        }
    }
}

bool logds::paramoneur::PruneRulesWhileCollectinMISes::retainRuleAsNonBot(const UltimateHornRule &rule) {
    std::set<int64_t> FV = rule.fvName();

    for (const auto& x : rule.tail) {
        if (x.isWeaklyNegated) {
            return false;
        }
    }

    if (env->isBottom(rule.head)) {
        // 1. se la regola termina con bot, allora tutte le sue possibili instanziazioni danno MIS.
        // (1. if the rule ends with bot, then all its possible instantiations yield MIS.)
        if (!FV.empty()) {
            std::vector<int64_t> lFV(FV.begin(), FV.end());
            const std::vector<int64_t> constants{env->constant_idx.begin(), env->constant_idx.end()};
            forEachTuple<int64_t>(constants, lFV.size(), [&](const std::vector<int64_t>& x) {
                UltimateHornRule r2 = env->instantiateRule(rule, zipToMap(lFV, x));
                expandMISOverGroundRule(r2);
                return false;
            });
            return false;
        } else {
            return expandMISOverGroundRule(rule);
        }
    } else {
        return true;
    }
}

struct PolarizedIndices {
    std::set<int> positiveBaseIndices;
    std::set<int> strongNegatedBaseIndices;
    std::set<int> nativeSignedIndices;
};

bool logds::paramoneur::isRuleTailInconsistentViaMis(
    const UltimateHornRule &rule,
    const std::vector<std::vector<int>> &collectionOfMis,
    int bVal,
    int maxLParam,
    int stopToken)
{
    std::vector<int> constantsUniverse;
    const auto& parser = GlobalEnvironmentProxy::getInstance();
    if (!parser.all_discovered_constants.empty()) {
        for (const auto& c : parser.all_discovered_constants) {
            constantsUniverse.push_back(c);   // str(c) on strings is the identity
        }
    }

    std::set<std::string> variableNames;
    for (const auto& literal : rule.tail) {
        for (const auto& arg : literal.atom->args) {
            if (arg.type == ArgType::Variable) {
                variableNames.insert(arg.name);
            }
        }
    }
    for (const auto& constraint : rule.constraints) {
        if (constraint.left.type == ArgType::Variable) {
            variableNames.insert(constraint.left.name);
        }
        if (constraint.right.type == ArgType::Variable) {
            variableNames.insert(constraint.right.name);
        }
    }
    std::vector<std::string> varList(variableNames.begin(), variableNames.end());   // sorted(list(variable_names))

    // 1. Predicato funzionale per convalidare i vincoli accessori e le uguaglianze transitive (Es. 3=Y & X=Z)
    // (1. Functional predicate to validate the accessory constraints and the transitive equalities (e.g. 3=Y & X=Z))
    auto validateBindingConstraints = [&](const std::vector<int>& pTuple) -> bool {
        std::unordered_map<std::string, int> binding = zipToMap(varList, pTuple);
        for (const auto& constraint : rule.constraints) {
            const auto valL = (constraint.left.type == ArgType::Variable)
                                         ? binding.at(constraint.left.name)
                                         : constraint.left.id;
            const auto valR = (constraint.right.type == ArgType::Variable)
                                         ? binding.at(constraint.right.name)
                                         : constraint.right.id;
            if (constraint.isEqual && valL != valR) {
                return false;
            }
            if (!constraint.isEqual && valL == valR) {
                return false;
            }
        }
        return true;
    };



    // 2. RETTIFICA SCOPE CORE: Estrazione atomica e unificata di tutti gli insiemi ground richiesti all'interno dello stesso blocco attivo
    // (2. CORE SCOPE FIX: atomic and unified extraction of all required ground sets within the same active block)
    auto extractAllPolarizedIndicesUnified = [&](const std::vector<int>& pTuple) -> PolarizedIndices {
        std::unordered_map<std::string, int> binding = zipToMap(varList, pTuple);
        PolarizedIndices out;

        for (const auto& literal : rule.tail) {
            ArbitraryAtom gAtom = literal.atom->instantiate_atom_with_binding(binding);

            // Calcolo dell'indice per l'autocontraddizione analitica (Caso 2: base comune a sign=0)
            // (Computing the index for analytical self-contradiction (Case 2: common base with sign=0))
            ArbitraryAtom gAtomPos = gAtom;   // ArbitraryAtom(sign=0, predicate, args)
            gAtomPos.sign = 0;
            int flatIdxPos = compile_atom_to_index_param(gAtomPos, bVal, maxLParam, stopToken);

            // Calcolo dell'indice nativo per il controllo dei MIS esterni (Caso 1: preserva il segno reale dell'atomo)
            // (Computing the native index for checking external MIS (Case 1: preserves the atom's real sign))
            int flatIdxNative = compile_atom_to_index_param(gAtom, bVal, maxLParam, stopToken);
            out.nativeSignedIndices.insert(flatIdxNative);

            if (gAtom.sign == 1) {
                out.strongNegatedBaseIndices.insert(flatIdxPos);
            } else {
                out.positiveBaseIndices.insert(flatIdxPos);
            }
        }

        return out;
    };

    // 3. Scansione ad attivazione short-circuit pigra esente da NameError o sfasamenti di contesto
    // (3. Lazy short-circuit scan free from NameError or context shifts)
    // filter(_validate_binding_constraints, raw_product_generator) followed by
    // map(_extract_all_polarized_indices_unified, ...) is realised by a single lazy loop.
    bool result = false;
    forEachTuple<int>(constantsUniverse, varList.size(), [&](const std::vector<int>& pTuple) -> bool {
        if (!validateBindingConstraints(pTuple)) {
            return false;
        }
        PolarizedIndices polarized = extractAllPolarizedIndicesUnified(pTuple);
        const std::set<int>& positiveSet = polarized.positiveBaseIndices;
        const std::set<int>& strongNegatedSet = polarized.strongNegatedBaseIndices;
        const std::set<int>& nativeSignedSet = polarized.nativeSignedIndices;

        // CASO 2: Controllo di Autocontraddizione Intrinseca per Strong Negation (A e ¬A coesistenti)
        // (CASE 2: Intrinsic self-contradiction check for Strong Negation (A and ¬A coexisting))
        bool intersects = false;
        if (!positiveSet.empty() && !strongNegatedSet.empty()) {
            for (int idx : positiveSet) {
                if (strongNegatedSet.count(idx) > 0) {
                    intersects = true;
                    break;
                }
            }
        }
        if (intersects) {
            std::cout << "   -> [STRONG NEGATION ANOMALY] Caught self-contradicting literal pair (A and ¬A) within the rule's own tail under current binding."
                    << std::endl;
            result = true;
            return true;
        }

        // CASO 1 RETTIFICATO: Verifica di compatibilità ed assimilazione rispetto a MIS esterni usando il set nativo già estratto
        // (CASE 1 CORRECTED: Compatibility and assimilation check against external MIS using the already extracted native set)
        if (!collectionOfMis.empty() && !nativeSignedSet.empty()) {
            for (const auto& misTuple : collectionOfMis) {
                bool allPresent = std::all_of(misTuple.begin(), misTuple.end(), [&](int misAtomIdx) {
                    return nativeSignedSet.count(misAtomIdx) > 0;
                });
                if (allPresent) {
                    result = true;
                    return true;
                }
            }
        }
        return false;
    });

    return result;
}

std::vector<logds::paramoneur::UltimateHornRule> logds::paramoneur::executeParaconsistentTheoryContraction(
    const std::vector<UltimateHornRule> &coreRulesList, const std::vector<std::vector<int>> &collectionOfMis, int bVal,
    int maxLParam, int stopToken) {
    std::vector<UltimateHornRule> purifiedRulesPool;
    int purgedCount = 0;
    for (const auto& rule : coreRulesList) {
        if (isRuleTailInconsistentViaMis(rule, collectionOfMis, bVal, maxLParam, stopToken)) {
            ++purgedCount;
            std::cout << "   [STRONG PARACONSISTENT CONTRACTION] Excising rule to block Ex Falso Quodlibet propagation from true contradictions."
                    << std::endl;
        } else {
            purifiedRulesPool.push_back(rule);
        }
    }
    if (purgedCount > 0) {
        std::cout << "   -> [PROCESS FINISHED] Purged " << purgedCount << " rules from training structures." << std::endl;
    }
    return purifiedRulesPool;
}
