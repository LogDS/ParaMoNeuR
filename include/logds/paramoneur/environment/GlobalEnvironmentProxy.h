//
// Created by gyankos on 22/10/26.
//

#ifndef PARAMONEUR2_GLOBALENVIRONMENTPROXY_H
#define PARAMONEUR2_GLOBALENVIRONMENTPROXY_H
#include <cassert>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "logds/paramoneur/language/compiled_structures.h"

namespace logds {
    namespace paramoneur {
        // ---------------------------------------------------------------------------
        // Global environment (external dependency, declared here)
        // TODO: this is just a STUB method!
        // ---------------------------------------------------------------------------
        class GlobalEnvironmentProxy {
            GlobalEnvironmentProxy() {}
        public:
            // Singleton accessor (provided by the paramoneur global environment module).
            static GlobalEnvironmentProxy& getInstance() {
                static GlobalEnvironmentProxy instance;
                if (!instance.init) {
                    instance.predicates_map[std::string(kBotSymbol)] = kPredicateFalsehood;
                    instance.reverse_predicates_map[kPredicateFalsehood] = std::string(kBotSymbol);
                    instance.init = true;
                }
                return instance;
            }
            ~GlobalEnvironmentProxy() = default;

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

            static bool isBottom(int x) {
                return  x == kPredicateFalsehood;
            }

            bool isBottom(const StrAtom& x) const{
                return (!x.isNegated) && (x.predicate == kBotSymbol);
            }

            bool isBottom(const ArbitraryAtom& atom) const {
                return (atom.predicate == kPredicateFalsehood) && (!atom.sign);
            }

            Argument instantiateArgument(const Argument & ref, const std::unordered_map<int, int> & pairs);
            Argument instantiateArgument(const Argument & ref, const std::unordered_map<int, std::string> & pairs);
            Argument instantiateArgument(const Argument & ref, const std::unordered_map<std::string, int> & pairs);
            ArbitraryAtom instantiateAtom(const ArbitraryAtom & head, const std::unordered_map<int, int> & pairs);
            BodyLiteral instantiateBodyLiteral(const BodyLiteral & ref, const std::unordered_map<int, int> & pairs);
            EqualityConstraint instantiateConstraint(const EqualityConstraint & ref, const std::unordered_map<int, int> &pairs);
            UltimateHornRule instantiateRule(const UltimateHornRule & rule, const std::unordered_map<int, int> & pairs);

            FlatIndex totalLogicalCoordinates = 0;
            int maxBodyRuleLength = 0;
            int maxSlotsBody = 0;
            int maxArity = 0;
            int maxVariablesPerRuleDetected = 0;
            std::unordered_map<std::string, int> varid{};
            std::unordered_map<int, std::string> reverse_predicates_map{}, constantsMap{}, constants_and_var_map_inv{}, reversePredicatesMap{};
            std::unordered_set<std::string> vars_strs{}, constant_strs{};
            std::unordered_map<std::string, int> predicate_arity{}, constants_and_var_map{}, predicates_map{};
            std::unordered_set<int> constant_idx{}, vars_idx{}, all_discovered_constants{};
            uint64_t max_constant_id = kStopToken +1;
            int max_arity = 0;
            int max_variables_per_rule_detected = 0;
            int max_body_rule_length = 0;
            int max_additional_slots = 10;
            /*        self.datasets: List[Any] = []  # Raccolta centralizzata di tutti i dataset compilati per l'inferenza multi-scenario
            self._initialized = True
            self.world_tracking = None*/
        private:
            bool init = false;
        };
    } // paramoneur
} // logds

#endif //PARAMONEUR2_GLOBALENVIRONMENTPROXY_H
