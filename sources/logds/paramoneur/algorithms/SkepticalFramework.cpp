//
// Created by gyankos on 23/10/26.
//

#include <logds/paramoneur/algorithms/SkepticalFramework.h>

logds::paramoneur::scikit_interface::PotentialResult logds::paramoneur::QueryOutput::fromType(
    const scikit_interface::Type t) {
    switch (t) {
        case scikit_interface::Type::Interval:
            return std::pair{skeptical_min_truth, optimistic_max_truth};

        case scikit_interface::Type::Proba:
            return average_truth;

        case scikit_interface::Type::Class:
            return outcome_classification;
    }

    return scikit_interface::OutcomeClass::Unknown;
}
