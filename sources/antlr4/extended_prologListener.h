
// Generated from extended_prolog.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "extended_prologParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by extended_prologParser.
 */
class  extended_prologListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProgram(extended_prologParser::ProgramContext *ctx) = 0;
  virtual void exitProgram(extended_prologParser::ProgramContext *ctx) = 0;

  virtual void enterLine(extended_prologParser::LineContext *ctx) = 0;
  virtual void exitLine(extended_prologParser::LineContext *ctx) = 0;

  virtual void enterRule_declaration(extended_prologParser::Rule_declarationContext *ctx) = 0;
  virtual void exitRule_declaration(extended_prologParser::Rule_declarationContext *ctx) = 0;

  virtual void enterWorld(extended_prologParser::WorldContext *ctx) = 0;
  virtual void exitWorld(extended_prologParser::WorldContext *ctx) = 0;

  virtual void enterFact_declaration(extended_prologParser::Fact_declarationContext *ctx) = 0;
  virtual void exitFact_declaration(extended_prologParser::Fact_declarationContext *ctx) = 0;

  virtual void enterHead_expr(extended_prologParser::Head_exprContext *ctx) = 0;
  virtual void exitHead_expr(extended_prologParser::Head_exprContext *ctx) = 0;

  virtual void enterIntegrity_constraint(extended_prologParser::Integrity_constraintContext *ctx) = 0;
  virtual void exitIntegrity_constraint(extended_prologParser::Integrity_constraintContext *ctx) = 0;

  virtual void enterFact_expr(extended_prologParser::Fact_exprContext *ctx) = 0;
  virtual void exitFact_expr(extended_prologParser::Fact_exprContext *ctx) = 0;

  virtual void enterBody_elements(extended_prologParser::Body_elementsContext *ctx) = 0;
  virtual void exitBody_elements(extended_prologParser::Body_elementsContext *ctx) = 0;

  virtual void enterBody_element(extended_prologParser::Body_elementContext *ctx) = 0;
  virtual void exitBody_element(extended_prologParser::Body_elementContext *ctx) = 0;

  virtual void enterLiteral_expr(extended_prologParser::Literal_exprContext *ctx) = 0;
  virtual void exitLiteral_expr(extended_prologParser::Literal_exprContext *ctx) = 0;

  virtual void enterStrEqualityConstraint(extended_prologParser::StrEqualityConstraintContext *ctx) = 0;
  virtual void exitStrEqualityConstraint(extended_prologParser::StrEqualityConstraintContext *ctx) = 0;

  virtual void enterArgTypeVARIABLE(extended_prologParser::ArgTypeVARIABLEContext *ctx) = 0;
  virtual void exitArgTypeVARIABLE(extended_prologParser::ArgTypeVARIABLEContext *ctx) = 0;

  virtual void enterArgTypeCONSTANT(extended_prologParser::ArgTypeCONSTANTContext *ctx) = 0;
  virtual void exitArgTypeCONSTANT(extended_prologParser::ArgTypeCONSTANTContext *ctx) = 0;

  virtual void enterEq_predicate(extended_prologParser::Eq_predicateContext *ctx) = 0;
  virtual void exitEq_predicate(extended_prologParser::Eq_predicateContext *ctx) = 0;

  virtual void enterNeq_predicate(extended_prologParser::Neq_predicateContext *ctx) = 0;
  virtual void exitNeq_predicate(extended_prologParser::Neq_predicateContext *ctx) = 0;

  virtual void enterAtom(extended_prologParser::AtomContext *ctx) = 0;
  virtual void exitAtom(extended_prologParser::AtomContext *ctx) = 0;

  virtual void enterArguments_list(extended_prologParser::Arguments_listContext *ctx) = 0;
  virtual void exitArguments_list(extended_prologParser::Arguments_listContext *ctx) = 0;


};

