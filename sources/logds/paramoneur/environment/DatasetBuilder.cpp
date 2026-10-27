//
// Created by gyankos on 24/10/26.
//

#include <logds/paramoneur/environment/GlobalEnvironmentProxy.h>
#include "logds/paramoneur/environment/DatasetBuilder.h"

#include "logds/paramoneur/algorithms/canonical_form.h"
#include "logds/paramoneur/algorithms/ParaconsistentTheoryContraction.h"

namespace logds {
    namespace paramoneur {
        GlobalEnvironmentProxy * Dataset::env() {
            return &GlobalEnvironmentProxy::getInstance();
        }

        void Dataset::post_process_rules() {
            auto& env = GlobalEnvironmentProxy::getInstance();
            /*  Ensuring that rules are not duplicated, thus providing a pre-processing
            * Still, returning just the indices of the relevant rules, so to make computation more efficient and allow
            */
            auto raw_rules_idx = prune_equivalent_rules_before_tensor_idx(raw_rules);

            /*Retaining all the rules not terminating with bot and, for the rest, generate integrity constraints after
             *grounding as MIS*/
            PruneRulesWhileCollectinMISes pruner{};
            std::erase_if(raw_rules_idx, [&pruner,this](int64_t x) {
                return !pruner.retainRuleAsNonBot(raw_rules[x]);
            });

            /* If all the possible rule activations contain a MIS, then discard the rule (None is returned)
             * Otherwise, just over-complicate the rule by adding the inequality constraints stating in which conditions
             * the rule does not hold
             */

            std::vector<UltimateHornRule> L;
            if (!pruner.MIS.empty()) {
                for (const auto& x : raw_rules_idx) {
                    auto expanded = pruner.removeRulesHavingMisInBody(raw_rules[x]);
                    if (expanded) {
                        if (expanded->tail.empty()) {
                            std::vector<ArbitraryAtom> v;
                            env.groundAtom(expanded->head, v);
                            for (const auto& x : v) {
                                raw_kb.emplace_back(x, 1.0);
                                string_kb.emplace_back(env.decompileAtom(x));
                                string_weights.emplace_back(1.0);
                            }
                        } else {
                            L.emplace_back(*expanded);
                        }
                    }
                }
                raw_rules_idx.clear();
                string_rules.clear();
                raw_rules.clear();
            } else {
                for (const auto& expanded : raw_rules) {
                    if (expanded.tail.empty()) {
                        std::vector<ArbitraryAtom> v;
                        env.groundAtom(expanded.head, v);
                        for (const auto& x: v) {
                            raw_kb.emplace_back(x, 1.0);
                            string_kb.emplace_back(env.decompileAtom(x));
                            string_weights.emplace_back(1.0);
                        }
                    } else {
                        L.emplace_back(expanded);
                    }
                }
            }

            string_rules.clear();
            std::transform(L.begin(), L.end(), std::back_inserter(string_rules), [&env](auto& x) {
                return env.decompileUltimateHornRule(x);
            });
            raw_rules = std::move(L);
            MIS = pruner.MIS;
        }

        Dataset DatasetBuilder::build() {
            auto& env = GlobalEnvironmentProxy::getInstance();
            auto pp = env.parseProgram(M);
            auto t = env.parseConcepts(kb, w);

            Dataset dataset_instance(pp, t, M, kb, w);
            dataset_instance.post_process_rules();

            env.addDataset(dataset_instance);
            return dataset_instance;
        }

    } // paramoneur
} // logds