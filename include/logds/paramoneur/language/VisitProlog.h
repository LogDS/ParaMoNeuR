//
// Created by gyankos on 24/10/26.
//

#ifndef PARAMONEUR2_VISITPROLOG_H
#define PARAMONEUR2_VISITPROLOG_H
#include "extended_prologVisitor.h"
#include "parsed_structures.h"
#include <yaucl/data/json.h>

namespace logds {
    namespace paramoneur {
        class VisitProlog : public extended_prologVisitor {
            std::vector<double> program_w;
            std::vector<HornRule> program;
        public:
            std::any visitProgram(extended_prologParser::ProgramContext *context) override {
                if (context) {
                    for (const auto& x : context->line()) {
                        visit(x);
                    }
                }
            }

            std::any visitLine(extended_prologParser::LineContext *context) override {
                if (context) {
                    if (context->rule_declaration()) {
                        const auto& [R, W] = std::any_cast<std::pair<HornRule, double>>(visit(context->rule_declaration()));
                        program_w.emplace_back(W);
                        program.emplace_back(R);
                    } else if (context->world()) {

                    }
                }
            }

            std::any visitRule_declaration(extended_prologParser::Rule_declarationContext *context) override;

            std::any visitWorld(extended_prologParser::WorldContext *context) override {
                
            }

            std::any visitFact_declaration(extended_prologParser::Fact_declarationContext *context) override;

            std::any visitHead_expr(extended_prologParser::Head_exprContext *context) override;

            std::any visitIntegrity_constraint(extended_prologParser::Integrity_constraintContext *context) override;

            std::any visitFact_expr(extended_prologParser::Fact_exprContext *context) override;

            std::any visitBody_elements(extended_prologParser::Body_elementsContext *context) override;

            std::any visitBody_element(extended_prologParser::Body_elementContext *context) override;

            std::any visitLiteral_expr(extended_prologParser::Literal_exprContext *context) override;

            std::any visitStrEqualityConstraint(extended_prologParser::StrEqualityConstraintContext *context) override;

            std::any visitArgTypeVARIABLE(extended_prologParser::ArgTypeVARIABLEContext *context) override;
            std::any visitArgTypeCONSTANT(extended_prologParser::ArgTypeCONSTANTContext *context) override;
            std::any visitEq_predicate(extended_prologParser::Eq_predicateContext *context) override;
            std::any visitNeq_predicate(extended_prologParser::Neq_predicateContext *context) override;
            std::any visitAtom(extended_prologParser::AtomContext *context) override;
        };
    } // paramoneur
} // logds

#endif //PARAMONEUR2_VISITPROLOG_H
