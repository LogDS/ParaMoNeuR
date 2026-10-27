//
// Created by gyankos on 24/10/26.
//

#include "logds/paramoneur/language/VisitProlog.h"

namespace logds {
    namespace paramoneur {
        std::any VisitProlog::visitArgTypeVARIABLE(extended_prologParser::ArgTypeVARIABLEContext *context) {
            if (context) {
                auto x = UNESCAPE(context->VARIABLE()->getText());
                return StrArgument {ArgType::Constant, x};
            }
            return std::nullopt;
        }

        std::any VisitProlog::visitArgTypeCONSTANT(extended_prologParser::ArgTypeCONSTANTContext *context) {
            if (context) {
                auto x = UNESCAPE(context->CONSTANT()->getText());
                return StrArgument {ArgType::Constant, x};
            }
            return std::nullopt;
        }

        std::any VisitProlog::visitEq_predicate(extended_prologParser::Eq_predicateContext *context) {
            return {true};
        }

        std::any VisitProlog::visitNeq_predicate(extended_prologParser::Neq_predicateContext *context) {
            return {false};
        }

        std::any VisitProlog::visitAtom(extended_prologParser::AtomContext *context) {
            if (context) {
                auto p = context->PREDICATE_NAME()->getText();
                std::vector<StrArgument> l;
                auto al = context->arguments_list();
                if (al) {
                    auto te = al->term_expr();
                    if (!te.empty()) {
                        for (const auto& x : te) {
                            auto val = visit(x);
                            if (val.type() == typeid(StrArgument)) {
                                l.push_back(std::any_cast<StrArgument>(val));
                            }
                        }
                    }
                }
                return StrAtom{false, p, l};
            }
            return std::nullopt;
        }
    } // paramoneur
} // logds