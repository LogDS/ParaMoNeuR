//
// Created by gyankos on 22/10/26.
//

#ifndef PARAMONEUR2_PARSED_STRUCTURES_H
#define PARAMONEUR2_PARSED_STRUCTURES_H
#include <ostream>
#include <string>
#include <vector>
#include <sstream>

#include "compiled_structures.h"
#include "logds/paramoneur/constants/constants.h"
#include "logds/paramoneur/types/shared.h"
#include "yaucl/strings/string_utils.h"

namespace logds::paramoneur {

    struct ConcreteStructures {

    };

    struct StrArgument : public  ConcreteStructures {
        ArgType type;
        std::string name;

        StrArgument(ArgType type, const std::string &name)
            : type(type),
              name(name) {
        }

        friend std::ostream & operator<<(std::ostream &os, const StrArgument &obj);
    };

    struct StrAtom : public ConcreteStructures {
        bool isNegated;
        std::string predicate;
        std::vector<StrArgument> args{};

        StrAtom(bool is_negated, const std::string &predicate, const std::vector<StrArgument> &args)
            : isNegated(is_negated),
              predicate(predicate),
              args(args) {
        }

        friend std::ostream & operator<<(std::ostream &os, const StrAtom &obj);

        StrAtom negate() const {
            if (!isNegated) {
                return StrAtom{true, predicate, args};
            }
            return *this;
        }

        // Source behavior preserved: when negated, this returns an atom that is still negated.
        StrAtom affirm() const {
            if (isNegated) {
                return StrAtom{true, predicate, args};
            }
            return *this;
        }
    };

    struct StrTailLiteral : public ConcreteStructures {
        StrAtom atom;
        bool isWeaklyNegated;

        StrTailLiteral(const StrAtom &atom, bool is_weakly_negated)
            : atom(atom),
              isWeaklyNegated(is_weakly_negated) {
        }

        friend std::ostream &operator<<(std::ostream &os, const StrTailLiteral &obj) {
            const std::string neg = obj.isWeaklyNegated ? std::string(kWeakNegationPrefix) : std::string();
            return os << neg << obj.atom;
        }
    };

    struct StrEqualityConstraint  : public ConcreteStructures{
        StrArgument left;
        StrArgument right;
        bool isEqual;

        StrEqualityConstraint(const StrArgument &left, const StrArgument &right, bool is_equal)
            : left(left),
              right(right),
              isEqual(is_equal) {
        }

        friend std::ostream & operator<<(std::ostream &os, const StrEqualityConstraint &obj){
            const std::string neg = obj.isEqual ? " == " : " != ";
            return os << obj.left <<  neg << obj.right;
        }
    };

    struct HornRule : public ConcreteStructures {
        StrAtom head;
        std::vector<StrTailLiteral> tail{};
        std::vector<StrEqualityConstraint> constraints{};
        double weight = 1.0;

        HornRule(const StrAtom &head, const std::vector<StrTailLiteral> &tail,
            const std::vector<StrEqualityConstraint> &constraints, double weight)
            : head(head),
              tail(tail),
              constraints(constraints),
              weight(weight) {
        }

        friend std::ostream & operator<<(std::ostream &os, const HornRule &obj){
            std::vector<std::string> tojoin;
            os   << std::to_string(obj.weight)
                        << " :: "
                        << obj.head
                        <<  " :- ";
            std::vector<std::string> serialized;
            for (const auto& constraint : obj.tail) {
                std::stringstream ss;
                ss << constraint;
            }
            for (const auto& constraint : obj.constraints) {
                std::stringstream ss;
                ss << constraint;
            }
            return os << yaucl::strings::join(serialized, ", ") << " . ";
        }
    };

}


#endif //PARAMONEUR2_PARSED_STRUCTURES_H
