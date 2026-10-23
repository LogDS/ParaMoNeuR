//
// Created by gyankos on 22/10/26.
//

#ifndef PARAMONEUR2_CONSTANTS_H
#define PARAMONEUR2_CONSTANTS_H

#include <array>
#include <string_view>

namespace logds::paramoneur {
    // ---------------------------------------------------------------------------
    // Constants
    // ---------------------------------------------------------------------------
    constexpr int kSignPositive = 0;
    constexpr int kSignStrongNeg = 1;
    constexpr int kStopToken = 0;
    constexpr int kPredicateFalsehood = 9;
    constexpr double kDefaultThreshold = 0.01;
    constexpr double kDefaultLogitFloor = -50.0;
    constexpr double kZeroOffsetCutoff = 0.01;
    constexpr double kContrastThreshold = 5.0;
    constexpr double kDefaultInfDensityWeight = 0.5;
    constexpr double kDefaultResidualInconsistencyWeight = 1.0;
    constexpr double kDefaultSkepcicismPenaltyWeight = 0.3;
    constexpr double kDefaultTemperature = 1.0;
    constexpr std::string_view kNegationSymbol = "¬";
    constexpr std::string_view kWeakNegationPrefix = "~ ";
    constexpr std::string_view kBotSymbol = "⊥";
    constexpr std::string_view kUnknownArgument = "X";
    constexpr std::string_view kPredFallbackPrefix = "pred_";
    constexpr std::array<std::string_view, 4> kContinuousFieldNames{"B_pred", "B_type", "B_bind", "A_head_pred"};

}

#endif //PARAMONEUR2_CONSTANTS_H
