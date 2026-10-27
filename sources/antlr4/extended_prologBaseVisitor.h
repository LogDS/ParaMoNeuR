
// Generated from extended_prolog.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "extended_prologVisitor.h"


/**
 * This class provides an empty implementation of extended_prologVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  extended_prologBaseVisitor : public extended_prologVisitor {
public:

  virtual std::any visitProgram(extended_prologParser::ProgramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLine(extended_prologParser::LineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRule_declaration(extended_prologParser::Rule_declarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWorld(extended_prologParser::WorldContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFact_declaration(extended_prologParser::Fact_declarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHead_expr(extended_prologParser::Head_exprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIntegrity_constraint(extended_prologParser::Integrity_constraintContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFact_expr(extended_prologParser::Fact_exprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBody_elements(extended_prologParser::Body_elementsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBody_element(extended_prologParser::Body_elementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLiteral_expr(extended_prologParser::Literal_exprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStrEqualityConstraint(extended_prologParser::StrEqualityConstraintContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArgTypeVARIABLE(extended_prologParser::ArgTypeVARIABLEContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArgTypeCONSTANT(extended_prologParser::ArgTypeCONSTANTContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEq_predicate(extended_prologParser::Eq_predicateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNeq_predicate(extended_prologParser::Neq_predicateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAtom(extended_prologParser::AtomContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArguments_list(extended_prologParser::Arguments_listContext *ctx) override {
    return visitChildren(ctx);
  }


};

