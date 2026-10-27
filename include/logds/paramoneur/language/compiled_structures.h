//
// Created by gyankos on 22/10/26.
//

#ifndef PARAMONEUR2_COMPILED_STRUCTURES_H
#define PARAMONEUR2_COMPILED_STRUCTURES_H
#include <memory>
#include <set>
#include <optional>
#include <unordered_map>

#include "parsed_structures.h"
#include "logds/paramoneur/types/aliases.h"
#include "logds/paramoneur/types/shared.h"

namespace logds::paramoneur {
    struct  ConcreteStructures;

    struct CompiledStructure {
        virtual std::set<int64_t> fvName() const  = 0;
        virtual std::shared_ptr<ConcreteStructures> strType() const = 0;
        virtual int64_t arity() const { return -1; }
        virtual std::set<int64_t> variables() const { return {}; }
    };

    struct Argument : public CompiledStructure {
        ArgType type;
        int64_t id;
        std::string name;

        Argument(ArgType type, int64_t id, const std::string &name)
            : type(type),
              id(id),
              name(name) {
        }

        Argument(const std::string &name, int64_t id,ArgType type )
            : type(type),
              id(id),
              name(name) {
        }


        VarOrConst varOrConstInternalRepr() const;
        std::set<int64_t> fvName() const override;
        std::shared_ptr<ConcreteStructures> strType() const override;

        int64_t arity() const override {
            return -1;
        }

        std::set<int64_t> variables() const override {
            if (type == ArgType::Variable) {
                return {id};
            }
            return {};
        }
    };

    struct ArbitraryAtom : public  CompiledStructure {
        int64_t sign;
        int64_t predicate;
        std::vector<Argument> args{};
        std::string name{};

        ArbitraryAtom(int64_t sign, int64_t predicate, const std::vector<Argument> &args, const std::string &name);

        ArbitraryAtom negated() const;
        ArbitraryAtom affirmative() const;
        std::pair<int, ArbitraryAtom> asSignAndPositiveAtom() const;
        std::shared_ptr<ConcreteStructures> strType() const override;
        std::set<int64_t> fvName() const override;

        int64_t arity() const override {
            return args.size();
        }

        std::set<int64_t> variables() const override {
                std::set<int64_t> s;
                for (const auto& x : args) {
                    const std::set<int64_t> part = x.variables();
                    s.insert(part.begin(), part.end());
                }
                return s;
        }

        ArbitraryAtom instantiate_atom_with_binding(const std::unordered_map<std::string, int>& variable_bindings) const;
    };


    struct BodyLiteral : public  CompiledStructure {
        // std::nullopt represents Python's atom = None.
        std::optional<ArbitraryAtom> atom{};
        bool isWeaklyNegated;

        BodyLiteral(bool is_weakly_negated)
    : atom(std::nullopt),
      isWeaklyNegated(is_weakly_negated) {
        }

        BodyLiteral(const ArbitraryAtom &atom, bool is_weakly_negated)
            : atom(atom),
              isWeaklyNegated(is_weakly_negated) {
        }

        std::set<int64_t> fvName() const override {
            return atom ? atom->fvName() : std::set<int64_t>{};
        }

        // Returns std::nullopt where Python would raise AttributeError (atom is None).
        std::shared_ptr<ConcreteStructures> strType() const override;

        std::set<int64_t> variables() const override {
            return atom ? atom->variables() : std::set<int64_t>{};
        }
    };


    struct EqualityConstraint  : public CompiledStructure {
        Argument left;
        Argument right;
        bool isEqual;

        EqualityConstraint(const Argument &left, const Argument &right, bool is_equal)
            : left(left),
              right(right),
              isEqual(is_equal) {
        }

        ConstraintTriplet varOrConstInternalRepr() const;

        bool eval() const {
            if (left.type == ArgType::Constant && right.type == ArgType::Constant) {
                return isEqual ? left.id == right.id : left.id != right.id;
            }
            return true;
        }

        std::set<int64_t> fvName() const override{
            auto result = left.fvName();
            const auto rightNames = right.fvName();
            result.insert(rightNames.begin(), rightNames.end());
            return result;
        }

        std::shared_ptr<ConcreteStructures> strType() const override;

        std::set<int64_t> variables() const override {
            std::set<int64_t> s = left.variables();
            const std::set<int64_t> rightVars = right.variables();
            s.insert(rightVars.begin(), rightVars.end());
            return s;
        }
    };

struct UltimateHornRule : public CompiledStructure {
    ArbitraryAtom head;
    std::vector<BodyLiteral> tail{};
    std::vector<EqualityConstraint> constraints{};
    bool isIntegrityConstraint = false;
    double weight = 1.0;

    UltimateHornRule(const ArbitraryAtom &head, const std::vector<BodyLiteral> &tail,
        const std::vector<EqualityConstraint> &constraints, bool is_integrity_constraint, double weight = 1.0)
        : head(head),
          tail(tail),
          constraints(constraints),
          isIntegrityConstraint(is_integrity_constraint),
          weight(weight) {
    }

    // Python's additionalConstraints=None default is represented by the empty default vector.
    UltimateHornRule updateConstraints(const std::vector<EqualityConstraint>& additionalConstraints = {}) const;

    std::size_t totalBodyLen() const {
        return tail.size() + constraints.size();
    }

    std::shared_ptr<ConcreteStructures>  strType() const override;

    int64_t arity() const override {
        int64_t a = head.arity();
        for (const auto& bodyLiteral : tail) {
            a = std::max(a, bodyLiteral.atom ? bodyLiteral.atom->arity() : -1);
        }
        return a;
    }

    std::set<int64_t> fvName() const override {
        auto result = head.fvName();
        for (const auto& literal : tail) {
            const auto part = literal.fvName();
            result.insert(part.begin(), part.end());
        }
        for (const auto& constraint : constraints) {
            const auto part = constraint.fvName();
            result.insert(part.begin(), part.end());
        }
        return result;
    }

    std::set<int64_t> variables() const override {
        std::set<int64_t> s = head.variables();
        for (const auto& x :tail) {
            const std::set<int64_t> part = x.variables();
            s.insert(part.begin(), part.end());
        }
        for (const auto& x : constraints) {
            const std::set<int64_t> part = x.variables();
            s.insert(part.begin(), part.end());
        }
        return s;
    }
};


}

#endif //PARAMONEUR2_COMPILED_STRUCTURES_H
