
// Generated from extended_prolog.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "extended_prologParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by extended_prologParser.
 */
class  extended_prologVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by extended_prologParser.
   */
    virtual std::any visitProgram(extended_prologParser::ProgramContext *context) = 0;

    virtual std::any visitLine(extended_prologParser::LineContext *context) = 0;

    virtual std::any visitRule_declaration(extended_prologParser::Rule_declarationContext *context) = 0;

    virtual std::any visitWorld(extended_prologParser::WorldContext *context) = 0;

    virtual std::any visitFact_declaration(extended_prologParser::Fact_declarationContext *context) = 0;

    virtual std::any visitHead_expr(extended_prologParser::Head_exprContext *context) = 0;

    virtual std::any visitIntegrity_constraint(extended_prologParser::Integrity_constraintContext *context) = 0;

    virtual std::any visitFact_expr(extended_prologParser::Fact_exprContext *context) = 0;

    virtual std::any visitBody_elements(extended_prologParser::Body_elementsContext *context) = 0;

    virtual std::any visitBody_element(extended_prologParser::Body_elementContext *context) = 0;

    virtual std::any visitLiteral_expr(extended_prologParser::Literal_exprContext *context) = 0;

    virtual std::any visitStrEqualityConstraint(extended_prologParser::StrEqualityConstraintContext *context) = 0;

    virtual std::any visitArgTypeVARIABLE(extended_prologParser::ArgTypeVARIABLEContext *context) = 0;

    virtual std::any visitArgTypeCONSTANT(extended_prologParser::ArgTypeCONSTANTContext *context) = 0;

    virtual std::any visitEq_predicate(extended_prologParser::Eq_predicateContext *context) = 0;

    virtual std::any visitNeq_predicate(extended_prologParser::Neq_predicateContext *context) = 0;

    virtual std::any visitAtom(extended_prologParser::AtomContext *context) = 0;

    virtual std::any visitArguments_list(extended_prologParser::Arguments_listContext *context) = 0;


};

