//
// Created by gyankos on 22/10/26.
//


#include "logds/paramoneur/environment/GlobalEnvironmentProxy.h"
#include "logds/paramoneur/language/compiled_structures.h"

logds::paramoneur::VarOrConst logds::paramoneur::Argument::varOrConstInternalRepr() const {
    return type == ArgType::Constant ? VarOrConst(id) : VarOrConst(name);
}

std::set<int64_t> logds::paramoneur::Argument::fvName() const {
    if (type == logds::paramoneur::ArgType::Constant) {
        return {};
    }
    return {id};
}

std::shared_ptr<logds::paramoneur::ConcreteStructures> logds::paramoneur::Argument::strType() const {
    return std::make_shared<StrArgument>(type, name);
}

logds::paramoneur::ArbitraryAtom::ArbitraryAtom(int64_t sign, int64_t predicate, const std::vector<Argument> &args,
                                                const std::string &name): sign(sign),
                                                                          predicate(predicate),
                                                                          args(args),
                                                                          name(name) {
}

logds::paramoneur::ArbitraryAtom logds::paramoneur::ArbitraryAtom::negated() const {
    if (sign == kSignPositive) {
        return ArbitraryAtom{kSignStrongNeg, predicate, args, name};
    }
    return *this;
}

logds::paramoneur::ArbitraryAtom logds::paramoneur::ArbitraryAtom::affirmative() const {
    if (sign == kSignStrongNeg) {
        return ArbitraryAtom{kSignPositive, predicate, args, name};
    }
    return *this;
}

std::pair<int, logds::paramoneur::ArbitraryAtom> logds::paramoneur::ArbitraryAtom::asSignAndPositiveAtom() const {
    return {sign, ArbitraryAtom{kSignPositive, predicate, args, name}};
}

std::shared_ptr<logds::paramoneur::ConcreteStructures> logds::paramoneur::ArbitraryAtom::strType() const {
    std::vector<StrArgument> converted;
    converted.reserve(args.size());
    for (const auto& arg : args) {
        converted.push_back(*static_cast<StrArgument *>(arg.strType().get()));
    }
    return std::make_shared<StrAtom>(sign == kSignStrongNeg, name, std::move(converted));
}

std::set<int64_t> logds::paramoneur::ArbitraryAtom::fvName() const {
    if (args.empty()) {
        return {};
    }
    std::set<int64_t> result;
    for (const auto& arg : args) {
        const std::set<int64_t> part = arg.fvName();
        result.insert(part.begin(), part.end());
    }
    return result;
}

logds::paramoneur::ArbitraryAtom logds::paramoneur::ArbitraryAtom::instantiate_atom_with_binding(
    const std::unordered_map<std::string, int> &variable_bindings) const {
    std::vector<Argument> ground_arguments;
    for (const auto& arg : args) {
        if (auto it = variable_bindings.find(arg.name); arg.type == ArgType::Variable && it != variable_bindings.end()) {
            ground_arguments.emplace_back(ArgType::Constant, it->second, std::to_string(it->second));
        } else {
            ground_arguments.emplace_back(arg);
        }
        return {sign, predicate, args, name};
    }
    return *this;
}

std::shared_ptr<logds::paramoneur::ConcreteStructures> logds::paramoneur::BodyLiteral::strType() const {
    if (!atom) {
        return nullptr;
    }
    return std::make_shared<StrTailLiteral>(*static_cast<StrAtom *>(atom->strType().get()), isWeaklyNegated);
}

logds::paramoneur::ConstraintTriplet logds::paramoneur::EqualityConstraint::varOrConstInternalRepr() const {
    return ConstraintTriplet{left.varOrConstInternalRepr(), right.varOrConstInternalRepr(), isEqual};
}

std::shared_ptr<logds::paramoneur::ConcreteStructures> logds::paramoneur::EqualityConstraint::strType() const {
    return std::make_shared<StrEqualityConstraint>(*(StrArgument*)left.strType().get(),
                                                   *(StrArgument*)right.strType().get(), isEqual);
}

logds::paramoneur::UltimateHornRule logds::paramoneur::UltimateHornRule::updateConstraints(
    const std::vector<EqualityConstraint> &additionalConstraints) const {
    std::set<ConstraintTriplet> uniqueReprs;
    for (const auto& constraint : constraints) {
        uniqueReprs.insert(constraint.varOrConstInternalRepr());
    }
    for (const auto& constraint : additionalConstraints) {
        uniqueReprs.insert(constraint.varOrConstInternalRepr());
    }
    std::vector<EqualityConstraint> rebuilt;
    rebuilt.reserve(uniqueReprs.size());
    for (const auto& triplet : uniqueReprs) {
        rebuilt.push_back(GlobalEnvironmentProxy::getInstance().equalityConstraintFromTriplet(triplet));
    }
    // Source behavior: the weight is not forwarded, so the default weight is used.
    return UltimateHornRule{head, tail, rebuilt, isIntegrityConstraint, weight};
}

std::shared_ptr<logds::paramoneur::ConcreteStructures> logds::paramoneur::UltimateHornRule::strType() const {
    std::vector<StrTailLiteral> convertedTail;
    for (const auto& literal : tail) {
        const auto converted = literal.strType();
        if (converted) {
            convertedTail.push_back(*(StrTailLiteral*)converted.get());
        }
    }
    std::vector<StrEqualityConstraint> convertedConstraints;
    convertedConstraints.reserve(constraints.size());
    for (const auto& constraint : constraints) {
        convertedConstraints.push_back(*(StrEqualityConstraint*)constraint.strType().get());
    }
    return std::make_shared<HornRule>(*(StrAtom*)head.strType().get(), convertedTail, convertedConstraints, weight);
}
