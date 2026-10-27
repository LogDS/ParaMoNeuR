//
// Created by gyankos on 22/10/26.
//

#ifndef PARAMONEUR2_GLOBALENVIRONMENTPROXY_H
#define PARAMONEUR2_GLOBALENVIRONMENTPROXY_H
#include <cassert>
#include <complex>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <ranges>

#include <yaucl/structures/setoids/basics.h>
#include "logds/paramoneur/language/compiled_structures.h"
#include "yaucl/structures/map_operations.h"


namespace logds::paramoneur {
    struct Dataset;
    // ---------------------------------------------------------------------------
    // Global environment (external dependency, declared here)
    // TODO: this is just a STUB method!
    // ---------------------------------------------------------------------------
    class GlobalEnvironmentProxy {
        GlobalEnvironmentProxy();
        ~GlobalEnvironmentProxy();

        int64_t get_or_create_predicate_id(const std::string& clean_name) {
            if (clean_name == kBotSymbol) {
                return kPredicateFalsehood;
            }
            auto it = predicates_map.find(clean_name);
            if (it != predicates_map.end()) {
                auto assigned_id = predicates_map.size()+1;
                while ((assigned_id == kPredicateFalsehood) ||
                    (assigned_id == kStopToken))
                    assigned_id++;
                it = predicates_map.emplace(clean_name, assigned_id).first;
                reverse_predicates_map.emplace(assigned_id, clean_name);
            }
            return it->second;
        }

        Argument parse_argument(const StrArgument& arg_str) {
            if (arg_str.type == ArgType::Variable) {
                auto it = constants_and_var_map.find(arg_str.name);
                std::size_t idx;
                if (it == constants_and_var_map.end()) {
                    max_constant_id += 1;
                    idx = constants_and_var_map.size();
                    it = constants_and_var_map.emplace(arg_str.name, idx).first;
                    constants_and_var_map_inv.emplace(idx, arg_str.name);
                    vars_idx.emplace(idx);
                    vars_strs.emplace(arg_str.name);
                } else {
                    idx = it->second;
                }
                return {arg_str.type, it->second, arg_str.name};
            } else {
                const auto& loc = arg_str.name;
                auto it = constants_and_var_map.find(loc);
                if (it == constants_and_var_map.end()) {
                    max_constant_id += 1;
                    it = constants_and_var_map.emplace(loc, max_constant_id).first;
                    constants_and_var_map_inv.emplace(max_constant_id, loc);
                    constant_idx.emplace(max_constant_id);
                }
                all_discovered_constants.emplace(it->second);
                constant_strs.emplace(loc);
                return {ArgType::Constant, it->second, loc};
            }
        }

    public:
        // Singleton accessor (provided by the paramoneur global environment module).
        static GlobalEnvironmentProxy& getInstance() {
            static GlobalEnvironmentProxy instance{};
            if (!instance.init) {
                instance.predicates_map[std::string(kBotSymbol)] = kPredicateFalsehood;
                instance.reverse_predicates_map[kPredicateFalsehood] = std::string(kBotSymbol);
                instance.init = true;
            }
            return instance;
        }
        std::optional<Argument> atomFromName(const std::string& id, bool isVarConstantOtherwise = true) const;
        std::optional<Argument> atomFromId(int id, bool isVarConstantOtherwise = true) const;

        // Rebuilds an EqualityConstraint from its internal triplet representation (provided by the module).
        EqualityConstraint equalityConstraintFromTriplet(const ConstraintTriplet& triplet) const;

        uint64_t compileAtomToIndex(const logds::paramoneur::ArbitraryAtom& g_atom);

        uint64_t max_B() const {
            return std::max({max_constant_id, predicates_map.size()+2, (uint64_t)kPredicateFalsehood})+2;
        }

        uint64_t max_L() const {
            return max_arity+2;
        }

        void addDataset(const Dataset* dataset);

        void groundAtom(const ArbitraryAtom & atom, std::vector<ArbitraryAtom>& result) {
            auto allVarIds = atom.fvName();
            if (allVarIds.empty()) {
                result.emplace_back(atom);
            } else {
                std::vector<int64_t> V{constant_idx.begin(), constant_idx.end()};
                forEachTuple<int64_t>(V,
                                allVarIds.size(),
                                [&](const std::vector<int64_t>& costanti_combo) {
                    auto d = zipToMap(V, costanti_combo);
                    result.emplace_back(instantiateAtom(atom, d));
                                    return false;
                });
            }
        }

        static bool isBottom(int x) {
            return  x == kPredicateFalsehood;
        }
        static bool isBottom(const StrAtom& x) {
            return (!x.isNegated) && (x.predicate == kBotSymbol);
        }
        static bool isBottom(const ArbitraryAtom& atom) {
            return (atom.predicate == kPredicateFalsehood) && (!atom.sign);
        }

        Argument instantiateArgument(const Argument & ref, const std::unordered_map<int64_t, int64_t> & pairs);
        Argument instantiateArgument(const Argument & ref, const std::unordered_map<int, std::string> & pairs);
        Argument instantiateArgument(const Argument & ref, const std::unordered_map<std::string, int> & pairs);
        ArbitraryAtom instantiateAtom(const ArbitraryAtom & head, const std::unordered_map<int64_t, int64_t> &pairs);
        BodyLiteral instantiateBodyLiteral(const BodyLiteral & ref, const std::unordered_map<int64_t, int64_t> & pairs);
        EqualityConstraint instantiateConstraint(const EqualityConstraint & ref, const std::unordered_map<int64_t, int64_t> &pairs);
        UltimateHornRule instantiateRule(const UltimateHornRule & rule, const std::unordered_map<int64_t, int64_t> & pairs);

        static StrArgument decompileArgument(const Argument& arg);
        StrAtom     decompileAtom(const ArbitraryAtom& atom) const;
        StrTailLiteral decompileTailLiteral(const BodyLiteral& literal) const;
        static StrEqualityConstraint decompileConstraint(const EqualityConstraint& constraint);
        HornRule decompileUltimateHornRule(const UltimateHornRule& u_rule) const;

        ArbitraryAtom parse_single_atom(const StrAtom & aa);
        BodyLiteral   parseBodyLiteral(const StrTailLiteral & aa);
        EqualityConstraint parseConstraint(const StrEqualityConstraint& aa);
        UltimateHornRule parseRule(const HornRule& horn_rule);
        std::vector<UltimateHornRule> parseProgram(const std::vector<HornRule>& program);
        std::vector<std::pair<ArbitraryAtom, double>>  parseConcepts(const std::vector<StrAtom>& str_atoms, const std::vector<double>& w_list);

        FlatIndex totalLogicalCoordinates = 0;
        int maxBodyRuleLength = 0;
        int maxSlotsBody = 0;
        int maxArity = 0;
        int maxVariablesPerRuleDetected = 0;
        std::unordered_map<std::string, int64_t> varid{};
        std::unordered_map<int64_t, std::string> reverse_predicates_map{}, constantsMap{}, constants_and_var_map_inv{}, reversePredicatesMap{};
        std::unordered_set<std::string> vars_strs{}, constant_strs{};
        std::unordered_map<std::string, int64_t> predicate_arity{}, constants_and_var_map{}, predicates_map{};
        std::unordered_set<int64_t> constant_idx{}, vars_idx{}, all_discovered_constants{};
        uint64_t max_constant_id = kStopToken +1;
        int max_arity = 0;
        uint64_t max_variables_per_rule_detected = 0;
        uint64_t max_body_rule_length = 0;
        int max_additional_slots = 10;
        std::vector<Dataset> datasets;
        /*        self.datasets: List[Any] = []  # Raccolta centralizzata di tutti i dataset compilati per l'inferenza multi-scenario
            self._initialized = True
            self.world_tracking = None*/
    private:
        bool init = false;
    };
}

#endif //PARAMONEUR2_GLOBALENVIRONMENTPROXY_H
