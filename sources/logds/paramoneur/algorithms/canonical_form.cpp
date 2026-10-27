//
// Created by gyankos on 23/10/26.
//

#include <map>
#include <logds/paramoneur/algorithms/canonical_form.h>

#include "logds/paramoneur/algorithms/unification.h"

// // Element of the body super-pool: kind tag, pointer to the original object, and signature
struct BodyElement {
    bool isLiteral;
    const logds::paramoneur::BodyLiteral* lit;
    const logds::paramoneur::EqualityConstraint* constr;
    std::string signature;
};

logds::paramoneur::UltimateHornRule getCanonicalAlphaForm(const logds::paramoneur::UltimateHornRule &rule) {
    // -------------------------------------------------------------------------
    // PHASE 1: LOCAL SIGNATURE AND OVERALL LEXICOGRAPHIC ORDERING (LITERALS + CONSTRAINTS)
    // -------------------------------------------------------------------------

    // Builds the local signature of a body literal (variables numbered by first appearance)
    auto buildLocalLiteralSignature = [](const logds::paramoneur::BodyLiteral& lit) -> std::string {
        std::map<std::string, std::string> localVarMap;
        std::string signPrefix = (lit.atom->sign == 1) ? "¬" : "";
        std::string weakPrefix = lit.isWeaklyNegated ? "~" : "";
        std::vector<std::string> argsSignatures;
        for (const logds::paramoneur::Argument& arg : lit.atom->args) {
            if (arg.type == logds::paramoneur::ArgType::Variable) {
                if (localVarMap.find(arg.name) == localVarMap.end()) {
                    // RHS is evaluated before insertion, so the index equals the current size
                    localVarMap[arg.name] = "X" + std::to_string(localVarMap.size());
                }
                argsSignatures.push_back(localVarMap[arg.name]);
            } else {
                argsSignatures.push_back("C_" + arg.name);
            }
        }
        return "PRED_" + weakPrefix + signPrefix + lit.atom->name + yaucl::strings::join(argsSignatures, "-", {}, "(", ")");
    };

    // Builds the local signature of an equality constraint
    auto buildLocalConstraintSignature = [](const logds::paramoneur::EqualityConstraint& c) -> std::string {
        // Using a set guarantees symmetry (Z=W and W=Z produce the same set)
        std::set<std::string> variablesSet{c.left.name, c.right.name};
        std::vector<std::string> sortedNames(variablesSet.begin(), variablesSet.end());
        std::map<std::string, std::string> localVarMap;
        for (std::size_t idx = 0; idx < sortedNames.size(); ++idx) {
            localVarMap[sortedNames[idx]] = "X" + std::to_string(idx);
        }
        std::vector<std::string> argsSignatures;
        for (const std::string& name : sortedNames) {
            argsSignatures.push_back(localVarMap[name]);
        }
        if (argsSignatures.size() == 1) {
            // Python original: args_signatures.append(args_signatures) appends the list to
            // itself; the subsequent '-'.join(...) raises TypeError
            // ("sequence item 1: expected str instance, list found").
            throw std::runtime_error(
                "TypeError: sequence item 1: expected str instance, list found");
        }
        std::string opType = c.isEqual ? "EQ" : "NEQ";
        return "CONSTR_" + opType + yaucl::strings::join(argsSignatures, "-", {}, "(", ")");
    };



    // Merge all body elements into the same super-pool for an invariant structural ordering
    std::vector<BodyElement> bodyElementsWithSignatures;
    for (const logds::paramoneur::BodyLiteral& lit : rule.tail) {
        bodyElementsWithSignatures.push_back(
            BodyElement{true, &lit, nullptr, buildLocalLiteralSignature(lit)});
    }
    for (const logds::paramoneur::EqualityConstraint& c : rule.constraints) {
        bodyElementsWithSignatures.push_back(
            BodyElement{false, nullptr, &c, buildLocalConstraintSignature(c)});
    }

    // Sort the list using only the signature string as key (stable, like Python's sort)
    std::stable_sort(bodyElementsWithSignatures.begin(), bodyElementsWithSignatures.end(),
                     [](const BodyElement& a, const BodyElement& b) {
                         return a.signature < b.signature;
                     });

    // -------------------------------------------------------------------------
    // PHASE 2: GLOBAL SEQUENTIAL CANONICAL ALPHA-RENAMING OVER THE ORDERED SPACE
    // -------------------------------------------------------------------------
    std::map<std::string, std::string> globalVariableMap;

    auto applyGlobalCanonicalMapping = [&globalVariableMap](const logds::paramoneur::ArbitraryAtom& atom) -> logds::paramoneur::ArbitraryAtom {
        std::vector<logds::paramoneur::Argument> canonicalArgs;
        for (const logds::paramoneur::Argument& arg : atom.args) {
            if (arg.type == logds::paramoneur::ArgType::Variable) {
                // For homogeneity with the constraints, the key for single variables in atoms is the plain string
                std::string varKey = arg.name;
                if (globalVariableMap.find(varKey) == globalVariableMap.end()) {
                    globalVariableMap[varKey] = "V" + std::to_string(globalVariableMap.size());
                }
                canonicalArgs.push_back(logds::paramoneur::Argument{logds::paramoneur::ArgType::Variable, arg.id, globalVariableMap[varKey]});
            } else {
                canonicalArgs.push_back(arg);
            }
        }
        return logds::paramoneur::ArbitraryAtom{atom.sign, atom.predicate, canonicalArgs, atom.name};
    };

    // 1. Rename the head (if the Horn clause is not a bot integrity constraint)
    logds::paramoneur::ArbitraryAtom canonicalHead = rule.isIntegrityConstraint
                                                         ? rule.head
                                                         : applyGlobalCanonicalMapping(rule.head);

    // 2. Traverse the ordered sequence, restoring and renaming both predicates and constraints
    std::vector<logds::paramoneur::BodyLiteral> canonicalTail;
    std::vector<logds::paramoneur::EqualityConstraint> finalReconstructedConstraints;

    for (const BodyElement& elem : bodyElementsWithSignatures) {
        if (elem.isLiteral) {
            logds::paramoneur::ArbitraryAtom canonicalAtom = applyGlobalCanonicalMapping(*elem.lit->atom);
            canonicalTail.emplace_back(canonicalAtom, elem.lit->isWeaklyNegated);
        } else {
            // Convert the ordered output into an immutable (hashable) sorted pair, as in the original
            std::vector<std::string> namesPool{elem.constr->left.name, elem.constr->right.name};
            std::sort(namesPool.begin(), namesPool.end());

            // If the variable is single (e.g. X==X), take the element; otherwise keep distinct pointers
            const std::string leftName = namesPool[0];
            const std::string rightName = namesPool.size() > 1 ? namesPool[1] : namesPool[0];

            if (globalVariableMap.find(leftName) == globalVariableMap.end()) {
                globalVariableMap[leftName] = "V" + std::to_string(globalVariableMap.size());
            }
            if (globalVariableMap.find(rightName) == globalVariableMap.end()) {
                globalVariableMap[rightName] = "V" + std::to_string(globalVariableMap.size());
            }

            const std::string vGlobLeft = globalVariableMap[leftName];
            const std::string vGlobRight = globalVariableMap[rightName];

            // RETENTION CRITERION: if the constraint unifies two distinct variables (non-redundant), keep it
            if (vGlobLeft != vGlobRight) {
                finalReconstructedConstraints.push_back(
                    logds::paramoneur::EqualityConstraint{
                        logds::paramoneur::Argument{logds::paramoneur::ArgType::Variable, elem.constr->left.id, vGlobLeft},
                        logds::paramoneur::Argument{logds::paramoneur::ArgType::Variable, elem.constr->right.id, vGlobRight},
                        elem.constr->isEqual});
            }
        }
    }

    return logds::paramoneur::UltimateHornRule{canonicalHead, canonicalTail, finalReconstructedConstraints,
        rule.isIntegrityConstraint, rule.weight};
}

std::vector<int64_t> prune_equivalent_rules_before_tensor_idx(
    const std::vector<logds::paramoneur::UltimateHornRule> &rule_list) {
    std::vector<logds::paramoneur::UltimateHornRule> canonical_rules;
    std::transform(rule_list.begin(), rule_list.end(), std::back_inserter(canonical_rules), getCanonicalAlphaForm);
    std::vector<int64_t> unique_retained_rules;
    for (uint64_t idx = 0; idx < canonical_rules.size(); ++idx) {
        bool isDuplicate = false;
        const auto& current_rule = canonical_rules[idx];
        for (const auto& idx_rule : unique_retained_rules) {
            if (logds::paramoneur::haveRulesSameBodyAndCompatibleHead(current_rule, rule_list[idx_rule], true).has_value()) {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate) {
            unique_retained_rules.emplace_back(idx);
        }
    }
    return unique_retained_rules;
}
