//
// Created by gyankos on 22/10/26.
//

#include "logds/paramoneur/environment/GlobalEnvironmentProxy.h"
#include "logds/paramoneur/environment/DatasetBuilder.h"
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

    void GlobalEnvironmentProxy::addDataset(const Dataset &dataset) {
        datasets.emplace_back(dataset);
    }

    Argument GlobalEnvironmentProxy::
    instantiateArgument(const Argument &ref, const std::unordered_map<int64_t, int64_t> &pairs) {
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
                                                          const std::unordered_map<int64_t, int64_t> &pairs) {
        if (head.predicate == kPredicateFalsehood || (head.name == kBotSymbol))
            return head;
        std::vector<Argument> bodyLiterals;
        for (const auto& ref : head.args)
            bodyLiterals.emplace_back(instantiateArgument(ref, pairs));
        return {head.sign, head.predicate, bodyLiterals, head.name};
    }

    BodyLiteral GlobalEnvironmentProxy::instantiateBodyLiteral(const BodyLiteral &ref,
        const std::unordered_map<int64_t, int64_t> &pairs) {
        if (ref.atom) {
            return {instantiateAtom(*ref.atom, pairs), ref.isWeaklyNegated};
        } else {
            return {ref.isWeaklyNegated};
        }
    }

    EqualityConstraint GlobalEnvironmentProxy::instantiateConstraint(const EqualityConstraint &ref,
        const std::unordered_map<int64_t, int64_t> &pairs) {
        return {instantiateArgument(ref.left, pairs), instantiateArgument(ref.right, pairs), ref.isEqual};
    }

    UltimateHornRule GlobalEnvironmentProxy::instantiateRule(const UltimateHornRule &rule,
                                                             const std::unordered_map<int64_t, int64_t> &pairs) {
        auto head = instantiateAtom(rule.head, pairs);
        std::vector<BodyLiteral> bodyLiterals;
        std::vector<EqualityConstraint> equalityConstraints;
        for (const auto& ref : rule.tail)
            bodyLiterals.push_back(instantiateBodyLiteral(ref, pairs));
        for (const auto& ref : rule.constraints)
            equalityConstraints.push_back(instantiateConstraint(ref, pairs));
        return {head, bodyLiterals, equalityConstraints, rule.isIntegrityConstraint, rule.weight};
    }

    StrArgument GlobalEnvironmentProxy::decompileArgument(const Argument &arg) {
        return {arg.type, arg.name};
    }

    StrAtom GlobalEnvironmentProxy::decompileAtom(const ArbitraryAtom &atom) const {
        /*        Converte un ArbitraryAtom numerico della GPU in un oggetto StrAtom
        ripristinando l'identità del predicato ed i rispettivi argomenti del primo ordine.*/
        std::string pred_name;
        // Recuperiamo il nome letterale del predicato dalla mappa invertita del Singleton
        auto it = reverse_predicates_map.find(atom.predicate);
        if (it == reverse_predicates_map.end()) {
            pred_name = "pred_"+ std::to_string(atom.predicate);
        } else {
            pred_name = it->second;
        }
        // Decompiliamo ricorsivamente l'intera lista degli argomenti associati
        std::vector<StrArgument> decompiled_args;
        std::transform(atom.args.begin(), atom.args.end(), std::back_inserter(decompiled_args), [&](const Argument& x) {
            return decompileArgument(x);
        });
        return StrAtom{ atom.sign==1, pred_name, decompiled_args};
    }

    StrTailLiteral GlobalEnvironmentProxy::decompileTailLiteral(const BodyLiteral &literal) const {
        return StrTailLiteral{decompileAtom(*literal.atom), literal.isWeaklyNegated};
    }

    StrEqualityConstraint GlobalEnvironmentProxy::decompileConstraint(const EqualityConstraint &constraint) {
        auto new_left_name = decompileArgument(constraint.left);
        auto new_right_name = decompileArgument(constraint.right);
        return {new_left_name, new_right_name, constraint.isEqual};
    }

    HornRule GlobalEnvironmentProxy::decompileUltimateHornRule(const UltimateHornRule &u_rule) const {
        /*        % \text{Universal Rule De-Linearization Factory: } \mathtt{UltimateHornRule} \longrightarrow \mathtt{HornRule}
        % Metodo principale di interfaccia: mappa un canovaccio UltimateHornRule rigenerando la HornRule originaria.*/
        /*        # 1. Decompilazione della testa (Consequence)
        # Se la testa fa riferimento all'atomo speciale 'bot', restituiamo l'istanza del BOT '⊥'*/
        StrAtom decompiled_head = bot;
        if (!isBottom(u_rule.head)) {
            decompiled_head = decompileAtom(u_rule.head);
        }

        std::vector<StrTailLiteral> decompiled_tail;
        std::transform(u_rule.tail.begin(), u_rule.tail.end(), std::back_inserter(decompiled_tail), [&](const BodyLiteral& lit) {
            return decompileTailLiteral(lit);
        });

        std::vector<StrEqualityConstraint> decompile_constraint;
        std::transform(u_rule.constraints.begin(), u_rule.constraints.end(), std::back_inserter(decompile_constraint), [&](const    EqualityConstraint& lit) {
            return decompileConstraint(lit);
        });

        return HornRule{decompiled_head, decompiled_tail, decompile_constraint, u_rule.weight};
    }

    ArbitraryAtom GlobalEnvironmentProxy::parse_single_atom(const StrAtom &aa) {
        if (aa.predicate == kBotSymbol) {
            return ArbitraryAtom(0, kPredicateFalsehood, {}, std::string(kBotSymbol));
        }
        int64_t pred_id = get_or_create_predicate_id(aa.predicate);
        std::vector<Argument> args_list;
        std::transform(aa.args.begin(), aa.args.end(), std::back_inserter(args_list), [&](const StrArgument& a) {
            return this->parse_argument(a);
        });
        if (!predicate_arity.contains(aa.predicate)) {
            predicate_arity[aa.predicate] = args_list.size();
        } else {
            predicate_arity[aa.predicate] = std::max((int64_t)args_list.size(), predicate_arity[aa.predicate]);
        }
        if (args_list.size() > max_arity) {
            max_arity = args_list.size();
        }
        return {aa.isNegated ? 1 : 0, pred_id, args_list, aa.predicate};
    }

    BodyLiteral GlobalEnvironmentProxy::parseBodyLiteral(const StrTailLiteral &aa) {
        return {parse_single_atom(aa.atom), aa.isWeaklyNegated};
    }

    EqualityConstraint GlobalEnvironmentProxy::parseConstraint(const StrEqualityConstraint &aa) {
        return {parse_argument(aa.left), parse_argument(aa.right), aa.isEqual};
    }

    UltimateHornRule GlobalEnvironmentProxy::parseRule(const HornRule &horn_rule) {
        auto head = parse_single_atom(horn_rule.head);
        auto is_integrity_constraint = (head.predicate == kPredicateFalsehood);

        std::vector<BodyLiteral> tail;
        std::transform(horn_rule.tail.begin(), horn_rule.tail.end(),
                       std::back_inserter(tail),
                       [&](const StrTailLiteral& x) {
                           return parseBodyLiteral(x);
                       });

        std::vector<EqualityConstraint> constr;
        std::transform(horn_rule.constraints.begin(), horn_rule.constraints.end(),
                       std::back_inserter(constr),
                       [&](const StrEqualityConstraint&x) {
                           return parseConstraint(x);
                       });

        UltimateHornRule result{head, tail, constr, is_integrity_constraint, horn_rule.weight};
        max_variables_per_rule_detected = std::max((uint64_t)max_variables_per_rule_detected,  (uint64_t)result.fvName().size());
        max_body_rule_length = std::max(max_body_rule_length, (uint64_t)tail.size());
        return result;
    }

    std::vector<UltimateHornRule> GlobalEnvironmentProxy::parseProgram(const std::vector<HornRule> &program) {
        std::vector<UltimateHornRule> result;
        std::transform(program.begin(), program.end(), std::back_inserter(result),
                       [&](const HornRule& x) {
                           return parseRule(x);
                       });
        return result;
    }

    std::vector<std::pair<ArbitraryAtom, double>> GlobalEnvironmentProxy::parseConcepts(
        const std::vector<StrAtom> &str_atoms, const std::vector<double> &w_list) {
        std::vector<std::pair<ArbitraryAtom, double>> result;
        result.reserve(str_atoms.size());
        for (auto&& [i, nome] : std::views::zip(str_atoms, w_list)) {
            result.emplace_back(parse_single_atom(i), nome);
        }
        return result;
    }
}
