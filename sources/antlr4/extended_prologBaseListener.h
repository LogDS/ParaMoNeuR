
// Generated from extended_prolog.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "extended_prologListener.h"


/**
 * This class provides an empty implementation of extended_prologListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  extended_prologBaseListener : public extended_prologListener {
public:

  virtual void enterProgram(extended_prologParser::ProgramContext * /*ctx*/) override { }
  virtual void exitProgram(extended_prologParser::ProgramContext * /*ctx*/) override { }

  virtual void enterLine(extended_prologParser::LineContext * /*ctx*/) override { }
  virtual void exitLine(extended_prologParser::LineContext * /*ctx*/) override { }

  virtual void enterRule_declaration(extended_prologParser::Rule_declarationContext * /*ctx*/) override { }
  virtual void exitRule_declaration(extended_prologParser::Rule_declarationContext * /*ctx*/) override { }

  virtual void enterWorld(extended_prologParser::WorldContext * /*ctx*/) override { }
  virtual void exitWorld(extended_prologParser::WorldContext * /*ctx*/) override { }

  virtual void enterFact_declaration(extended_prologParser::Fact_declarationContext * /*ctx*/) override { }
  virtual void exitFact_declaration(extended_prologParser::Fact_declarationContext * /*ctx*/) override { }

  virtual void enterHead_expr(extended_prologParser::Head_exprContext * /*ctx*/) override { }
  virtual void exitHead_expr(extended_prologParser::Head_exprContext * /*ctx*/) override { }

  virtual void enterIntegrity_constraint(extended_prologParser::Integrity_constraintContext * /*ctx*/) override { }
  virtual void exitIntegrity_constraint(extended_prologParser::Integrity_constraintContext * /*ctx*/) override { }

  virtual void enterFact_expr(extended_prologParser::Fact_exprContext * /*ctx*/) override { }
  virtual void exitFact_expr(extended_prologParser::Fact_exprContext * /*ctx*/) override { }

  virtual void enterBody_elements(extended_prologParser::Body_elementsContext * /*ctx*/) override { }
  virtual void exitBody_elements(extended_prologParser::Body_elementsContext * /*ctx*/) override { }

  virtual void enterBody_element(extended_prologParser::Body_elementContext * /*ctx*/) override { }
  virtual void exitBody_element(extended_prologParser::Body_elementContext * /*ctx*/) override { }

  virtual void enterLiteral_expr(extended_prologParser::Literal_exprContext * /*ctx*/) override { }
  virtual void exitLiteral_expr(extended_prologParser::Literal_exprContext * /*ctx*/) override { }

  virtual void enterStrEqualityConstraint(extended_prologParser::StrEqualityConstraintContext * /*ctx*/) override { }
  virtual void exitStrEqualityConstraint(extended_prologParser::StrEqualityConstraintContext * /*ctx*/) override { }

  virtual void enterArgTypeVARIABLE(extended_prologParser::ArgTypeVARIABLEContext * /*ctx*/) override { }
  virtual void exitArgTypeVARIABLE(extended_prologParser::ArgTypeVARIABLEContext * /*ctx*/) override { }

  virtual void enterArgTypeCONSTANT(extended_prologParser::ArgTypeCONSTANTContext * /*ctx*/) override { }
  virtual void exitArgTypeCONSTANT(extended_prologParser::ArgTypeCONSTANTContext * /*ctx*/) override { }

  virtual void enterEq_predicate(extended_prologParser::Eq_predicateContext * /*ctx*/) override { }
  virtual void exitEq_predicate(extended_prologParser::Eq_predicateContext * /*ctx*/) override { }

  virtual void enterNeq_predicate(extended_prologParser::Neq_predicateContext * /*ctx*/) override { }
  virtual void exitNeq_predicate(extended_prologParser::Neq_predicateContext * /*ctx*/) override { }

  virtual void enterAtom(extended_prologParser::AtomContext * /*ctx*/) override { }
  virtual void exitAtom(extended_prologParser::AtomContext * /*ctx*/) override { }

  virtual void enterArguments_list(extended_prologParser::Arguments_listContext * /*ctx*/) override { }
  virtual void exitArguments_list(extended_prologParser::Arguments_listContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

