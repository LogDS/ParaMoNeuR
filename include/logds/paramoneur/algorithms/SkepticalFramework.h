//
// Created by gyankos on 23/10/26.
//

#ifndef PARAMONEUR2_SKEPTICALFRAMEWORK_H
#define PARAMONEUR2_SKEPTICALFRAMEWORK_H
#include "logds/paramoneur/scikit_interface/scikit_interface_basetypes.h"

namespace logds::paramoneur {
    /**
     * This class represents the aggregated outputs that can be returned as a query outcome, in its
     * non-aggregated form. Then, depending on the expected outcome class, these will be represented
     * in the desired representation.
     */
    struct QueryOutput {
        double world_representativeness;
        double skeptical_min_truth;
        double optimistic_max_truth;
        double average_truth;
        // weighed_solutions_report: dict
        scikit_interface::OutcomeClass outcome_classification;

        scikit_interface::PotentialResult fromType(const scikit_interface::Type t);
    };
}

#endif //PARAMONEUR2_SKEPTICALFRAMEWORK_H
