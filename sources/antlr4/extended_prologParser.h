
// Generated from extended_prolog.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  extended_prologParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, WEAK_NEGATION_SIGN = 13, 
    NEGATION_SIGN = 14, VARIABLE = 15, WORLD_NAME = 16, PREDICATE_NAME = 17, 
    CONSTANT = 18, NUMBER = 19, SPACE = 20, COMMENT = 21, LINE_COMMENT = 22
  };

  enum {
    RuleProgram = 0, RuleLine = 1, RuleRule_declaration = 2, RuleWorld = 3, 
    RuleFact_declaration = 4, RuleHead_expr = 5, RuleIntegrity_constraint = 6, 
    RuleFact_expr = 7, RuleBody_elements = 8, RuleBody_element = 9, RuleLiteral_expr = 10, 
    RuleConstraint_expr = 11, RuleTerm_expr = 12, RuleOperator_sign = 13, 
    RuleAtom = 14, RuleArguments_list = 15
  };

  explicit extended_prologParser(antlr4::TokenStream *input);

  extended_prologParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~extended_prologParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class ProgramContext;
  class LineContext;
  class Rule_declarationContext;
  class WorldContext;
  class Fact_declarationContext;
  class Head_exprContext;
  class Integrity_constraintContext;
  class Fact_exprContext;
  class Body_elementsContext;
  class Body_elementContext;
  class Literal_exprContext;
  class Constraint_exprContext;
  class Term_exprContext;
  class Operator_signContext;
  class AtomContext;
  class Arguments_listContext; 

  class  ProgramContext : public antlr4::ParserRuleContext {
  public:
    ProgramContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<LineContext *> line();
    LineContext* line(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ProgramContext* program();

  class  LineContext : public antlr4::ParserRuleContext {
  public:
    LineContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Rule_declarationContext *rule_declaration();
    WorldContext *world();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LineContext* line();

  class  Rule_declarationContext : public antlr4::ParserRuleContext {
  public:
    Rule_declarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Head_exprContext *head_expr();
    Body_elementsContext *body_elements();
    antlr4::tree::TerminalNode *NUMBER();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Rule_declarationContext* rule_declaration();

  class  WorldContext : public antlr4::ParserRuleContext {
  public:
    WorldContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *WORLD_NAME();
    std::vector<Fact_declarationContext *> fact_declaration();
    Fact_declarationContext* fact_declaration(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  WorldContext* world();

  class  Fact_declarationContext : public antlr4::ParserRuleContext {
  public:
    Fact_declarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Fact_exprContext *fact_expr();
    antlr4::tree::TerminalNode *NUMBER();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Fact_declarationContext* fact_declaration();

  class  Head_exprContext : public antlr4::ParserRuleContext {
  public:
    Head_exprContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Integrity_constraintContext *integrity_constraint();
    Fact_exprContext *fact_expr();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Head_exprContext* head_expr();

  class  Integrity_constraintContext : public antlr4::ParserRuleContext {
  public:
    Integrity_constraintContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Integrity_constraintContext* integrity_constraint();

  class  Fact_exprContext : public antlr4::ParserRuleContext {
  public:
    Fact_exprContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AtomContext *atom();
    antlr4::tree::TerminalNode *NEGATION_SIGN();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Fact_exprContext* fact_expr();

  class  Body_elementsContext : public antlr4::ParserRuleContext {
  public:
    Body_elementsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Body_elementContext *> body_element();
    Body_elementContext* body_element(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Body_elementsContext* body_elements();

  class  Body_elementContext : public antlr4::ParserRuleContext {
  public:
    Body_elementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Literal_exprContext *literal_expr();
    Constraint_exprContext *constraint_expr();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Body_elementContext* body_element();

  class  Literal_exprContext : public antlr4::ParserRuleContext {
  public:
    Literal_exprContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AtomContext *atom();
    antlr4::tree::TerminalNode *WEAK_NEGATION_SIGN();
    antlr4::tree::TerminalNode *NEGATION_SIGN();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Literal_exprContext* literal_expr();

  class  Constraint_exprContext : public antlr4::ParserRuleContext {
  public:
    Constraint_exprContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Constraint_exprContext() = default;
    void copyFrom(Constraint_exprContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  StrEqualityConstraintContext : public Constraint_exprContext {
  public:
    StrEqualityConstraintContext(Constraint_exprContext *ctx);

    std::vector<Term_exprContext *> term_expr();
    Term_exprContext* term_expr(size_t i);
    Operator_signContext *operator_sign();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  Constraint_exprContext* constraint_expr();

  class  Term_exprContext : public antlr4::ParserRuleContext {
  public:
    Term_exprContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Term_exprContext() = default;
    void copyFrom(Term_exprContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ArgTypeCONSTANTContext : public Term_exprContext {
  public:
    ArgTypeCONSTANTContext(Term_exprContext *ctx);

    antlr4::tree::TerminalNode *CONSTANT();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ArgTypeVARIABLEContext : public Term_exprContext {
  public:
    ArgTypeVARIABLEContext(Term_exprContext *ctx);

    antlr4::tree::TerminalNode *VARIABLE();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  Term_exprContext* term_expr();

  class  Operator_signContext : public antlr4::ParserRuleContext {
  public:
    Operator_signContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Operator_signContext() = default;
    void copyFrom(Operator_signContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  Eq_predicateContext : public Operator_signContext {
  public:
    Eq_predicateContext(Operator_signContext *ctx);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  Neq_predicateContext : public Operator_signContext {
  public:
    Neq_predicateContext(Operator_signContext *ctx);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  Operator_signContext* operator_sign();

  class  AtomContext : public antlr4::ParserRuleContext {
  public:
    AtomContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *PREDICATE_NAME();
    Arguments_listContext *arguments_list();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AtomContext* atom();

  class  Arguments_listContext : public antlr4::ParserRuleContext {
  public:
    Arguments_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Term_exprContext *> term_expr();
    Term_exprContext* term_expr(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Arguments_listContext* arguments_list();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

