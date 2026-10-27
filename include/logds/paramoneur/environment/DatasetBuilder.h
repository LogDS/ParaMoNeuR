//
// Created by gyankos on 24/10/26.
//

#ifndef PARAMONEUR2_DATASETBUILDER_H
#define PARAMONEUR2_DATASETBUILDER_H
#include <unordered_set>

#include <yaucl/hashing/vector_hash.h>
#include "logds/paramoneur/language/parsed_structures.h"

namespace logds::paramoneur {
    class GlobalEnvironmentProxy;

    struct Dataset {
        std::vector<UltimateHornRule> raw_rules;
        std::vector<std::pair<ArbitraryAtom, double>> raw_kb;
        std::vector<HornRule> string_rules;
        std::vector<StrAtom> string_kb;
        std::vector<double> string_weights;
        bool is_pp_alpha{false};
        std::unordered_map<uint64_t, std::vector<uint64_t>> rule_bodies_map;
        std::unordered_set<std::vector<int64_t>> MIS;

        static GlobalEnvironmentProxy* env();
        bool empty() const { return string_kb.empty() && string_rules.empty(); }

        void post_process_rules();
    };

    class DatasetBuilder {
        std::vector<HornRule> M;
        std::vector<StrAtom> kb;
        std::vector<double>   w;

    public:
        void addRule(const HornRule& r, double weight = 1.0) {
            M.emplace_back(r).weight = weight;
        }

        void addConcept(const StrAtom& c, double weight = 1.0) {
            w.emplace_back(weight);
            kb.emplace_back(c);
        }

        Dataset build();
    };
}

#endif //PARAMONEUR2_DATASETBUILDER_H
