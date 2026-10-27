
// Generated from extended_prolog.g4 by ANTLR 4.13.2


#include "extended_prologListener.h"
#include "extended_prologVisitor.h"

#include "extended_prologParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct Extended_prologParserStaticData final {
  Extended_prologParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  Extended_prologParserStaticData(const Extended_prologParserStaticData&) = delete;
  Extended_prologParserStaticData(Extended_prologParserStaticData&&) = delete;
  Extended_prologParserStaticData& operator=(const Extended_prologParserStaticData&) = delete;
  Extended_prologParserStaticData& operator=(Extended_prologParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag extended_prologParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<Extended_prologParserStaticData> extended_prologParserStaticData = nullptr;

void extended_prologParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (extended_prologParserStaticData != nullptr) {
    return;
  }
#else
  assert(extended_prologParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<Extended_prologParserStaticData>(
    std::vector<std::string>{
      "program", "line", "rule_declaration", "world", "fact_declaration", 
      "head_expr", "integrity_constraint", "fact_expr", "body_elements", 
      "body_element", "literal_expr", "constraint_expr", "term_expr", "operator_sign", 
      "atom", "arguments_list"
    },
    std::vector<std::string>{
      "", "'::'", "':-'", "'.'", "'false'", "'falsehood'", "'fail'", "'\\u22A5'", 
      "','", "'=='", "'!='", "'('", "')'", "'~'", "'\\u00AC'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "WEAK_NEGATION_SIGN", 
      "NEGATION_SIGN", "VARIABLE", "WORLD_NAME", "PREDICATE_NAME", "CONSTANT", 
      "NUMBER", "SPACE", "COMMENT", "LINE_COMMENT"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,22,124,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,1,0,4,0,34,8,0,11,0,12,0,35,1,1,1,1,3,1,40,8,1,1,2,1,2,3,
  	2,44,8,2,1,2,1,2,1,2,1,2,1,2,1,3,1,3,5,3,53,8,3,10,3,12,3,56,9,3,1,3,
  	1,3,1,4,1,4,3,4,62,8,4,1,4,1,4,1,5,1,5,3,5,68,8,5,1,6,1,6,1,7,3,7,73,
  	8,7,1,7,1,7,1,8,1,8,1,8,5,8,80,8,8,10,8,12,8,83,9,8,1,9,1,9,3,9,87,8,
  	9,1,10,3,10,90,8,10,1,10,3,10,93,8,10,1,10,1,10,1,11,1,11,1,11,1,11,1,
  	12,1,12,3,12,103,8,12,1,13,1,13,3,13,107,8,13,1,14,1,14,1,14,1,14,1,14,
  	3,14,114,8,14,1,15,1,15,1,15,5,15,119,8,15,10,15,12,15,122,9,15,1,15,
  	0,0,16,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,0,1,1,0,4,7,122,0,33,
  	1,0,0,0,2,39,1,0,0,0,4,43,1,0,0,0,6,50,1,0,0,0,8,61,1,0,0,0,10,67,1,0,
  	0,0,12,69,1,0,0,0,14,72,1,0,0,0,16,76,1,0,0,0,18,86,1,0,0,0,20,89,1,0,
  	0,0,22,96,1,0,0,0,24,102,1,0,0,0,26,106,1,0,0,0,28,108,1,0,0,0,30,115,
  	1,0,0,0,32,34,3,2,1,0,33,32,1,0,0,0,34,35,1,0,0,0,35,33,1,0,0,0,35,36,
  	1,0,0,0,36,1,1,0,0,0,37,40,3,4,2,0,38,40,3,6,3,0,39,37,1,0,0,0,39,38,
  	1,0,0,0,40,3,1,0,0,0,41,42,5,19,0,0,42,44,5,1,0,0,43,41,1,0,0,0,43,44,
  	1,0,0,0,44,45,1,0,0,0,45,46,3,10,5,0,46,47,5,2,0,0,47,48,3,16,8,0,48,
  	49,5,3,0,0,49,5,1,0,0,0,50,54,5,16,0,0,51,53,3,8,4,0,52,51,1,0,0,0,53,
  	56,1,0,0,0,54,52,1,0,0,0,54,55,1,0,0,0,55,57,1,0,0,0,56,54,1,0,0,0,57,
  	58,5,3,0,0,58,7,1,0,0,0,59,60,5,19,0,0,60,62,5,1,0,0,61,59,1,0,0,0,61,
  	62,1,0,0,0,62,63,1,0,0,0,63,64,3,14,7,0,64,9,1,0,0,0,65,68,3,12,6,0,66,
  	68,3,14,7,0,67,65,1,0,0,0,67,66,1,0,0,0,68,11,1,0,0,0,69,70,7,0,0,0,70,
  	13,1,0,0,0,71,73,5,14,0,0,72,71,1,0,0,0,72,73,1,0,0,0,73,74,1,0,0,0,74,
  	75,3,28,14,0,75,15,1,0,0,0,76,81,3,18,9,0,77,78,5,8,0,0,78,80,3,18,9,
  	0,79,77,1,0,0,0,80,83,1,0,0,0,81,79,1,0,0,0,81,82,1,0,0,0,82,17,1,0,0,
  	0,83,81,1,0,0,0,84,87,3,20,10,0,85,87,3,22,11,0,86,84,1,0,0,0,86,85,1,
  	0,0,0,87,19,1,0,0,0,88,90,5,13,0,0,89,88,1,0,0,0,89,90,1,0,0,0,90,92,
  	1,0,0,0,91,93,5,14,0,0,92,91,1,0,0,0,92,93,1,0,0,0,93,94,1,0,0,0,94,95,
  	3,28,14,0,95,21,1,0,0,0,96,97,3,24,12,0,97,98,3,26,13,0,98,99,3,24,12,
  	0,99,23,1,0,0,0,100,103,5,15,0,0,101,103,5,18,0,0,102,100,1,0,0,0,102,
  	101,1,0,0,0,103,25,1,0,0,0,104,107,5,9,0,0,105,107,5,10,0,0,106,104,1,
  	0,0,0,106,105,1,0,0,0,107,27,1,0,0,0,108,113,5,17,0,0,109,110,5,11,0,
  	0,110,111,3,30,15,0,111,112,5,12,0,0,112,114,1,0,0,0,113,109,1,0,0,0,
  	113,114,1,0,0,0,114,29,1,0,0,0,115,120,3,24,12,0,116,117,5,8,0,0,117,
  	119,3,24,12,0,118,116,1,0,0,0,119,122,1,0,0,0,120,118,1,0,0,0,120,121,
  	1,0,0,0,121,31,1,0,0,0,122,120,1,0,0,0,15,35,39,43,54,61,67,72,81,86,
  	89,92,102,106,113,120
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  extended_prologParserStaticData = std::move(staticData);
}

}

extended_prologParser::extended_prologParser(TokenStream *input) : extended_prologParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

extended_prologParser::extended_prologParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  extended_prologParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *extended_prologParserStaticData->atn, extended_prologParserStaticData->decisionToDFA, extended_prologParserStaticData->sharedContextCache, options);
}

extended_prologParser::~extended_prologParser() {
  delete _interpreter;
}

const atn::ATN& extended_prologParser::getATN() const {
  return *extended_prologParserStaticData->atn;
}

std::string extended_prologParser::getGrammarFileName() const {
  return "extended_prolog.g4";
}

const std::vector<std::string>& extended_prologParser::getRuleNames() const {
  return extended_prologParserStaticData->ruleNames;
}

const dfa::Vocabulary& extended_prologParser::getVocabulary() const {
  return extended_prologParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView extended_prologParser::getSerializedATN() const {
  return extended_prologParserStaticData->serializedATN;
}


//----------------- ProgramContext ------------------------------------------------------------------

extended_prologParser::ProgramContext::ProgramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<extended_prologParser::LineContext *> extended_prologParser::ProgramContext::line() {
  return getRuleContexts<extended_prologParser::LineContext>();
}

extended_prologParser::LineContext* extended_prologParser::ProgramContext::line(size_t i) {
  return getRuleContext<extended_prologParser::LineContext>(i);
}


size_t extended_prologParser::ProgramContext::getRuleIndex() const {
  return extended_prologParser::RuleProgram;
}

void extended_prologParser::ProgramContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterProgram(this);
}

void extended_prologParser::ProgramContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitProgram(this);
}


std::any extended_prologParser::ProgramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitProgram(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::ProgramContext* extended_prologParser::program() {
  ProgramContext *_localctx = _tracker.createInstance<ProgramContext>(_ctx, getState());
  enterRule(_localctx, 0, extended_prologParser::RuleProgram);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(33); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(32);
      line();
      setState(35); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 737520) != 0));
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LineContext ------------------------------------------------------------------

extended_prologParser::LineContext::LineContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

extended_prologParser::Rule_declarationContext* extended_prologParser::LineContext::rule_declaration() {
  return getRuleContext<extended_prologParser::Rule_declarationContext>(0);
}

extended_prologParser::WorldContext* extended_prologParser::LineContext::world() {
  return getRuleContext<extended_prologParser::WorldContext>(0);
}


size_t extended_prologParser::LineContext::getRuleIndex() const {
  return extended_prologParser::RuleLine;
}

void extended_prologParser::LineContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLine(this);
}

void extended_prologParser::LineContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLine(this);
}


std::any extended_prologParser::LineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitLine(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::LineContext* extended_prologParser::line() {
  LineContext *_localctx = _tracker.createInstance<LineContext>(_ctx, getState());
  enterRule(_localctx, 2, extended_prologParser::RuleLine);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(39);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case extended_prologParser::T__3:
      case extended_prologParser::T__4:
      case extended_prologParser::T__5:
      case extended_prologParser::T__6:
      case extended_prologParser::NEGATION_SIGN:
      case extended_prologParser::PREDICATE_NAME:
      case extended_prologParser::NUMBER: {
        enterOuterAlt(_localctx, 1);
        setState(37);
        rule_declaration();
        break;
      }

      case extended_prologParser::WORLD_NAME: {
        enterOuterAlt(_localctx, 2);
        setState(38);
        world();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Rule_declarationContext ------------------------------------------------------------------

extended_prologParser::Rule_declarationContext::Rule_declarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

extended_prologParser::Head_exprContext* extended_prologParser::Rule_declarationContext::head_expr() {
  return getRuleContext<extended_prologParser::Head_exprContext>(0);
}

extended_prologParser::Body_elementsContext* extended_prologParser::Rule_declarationContext::body_elements() {
  return getRuleContext<extended_prologParser::Body_elementsContext>(0);
}

tree::TerminalNode* extended_prologParser::Rule_declarationContext::NUMBER() {
  return getToken(extended_prologParser::NUMBER, 0);
}


size_t extended_prologParser::Rule_declarationContext::getRuleIndex() const {
  return extended_prologParser::RuleRule_declaration;
}

void extended_prologParser::Rule_declarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRule_declaration(this);
}

void extended_prologParser::Rule_declarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRule_declaration(this);
}


std::any extended_prologParser::Rule_declarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitRule_declaration(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Rule_declarationContext* extended_prologParser::rule_declaration() {
  Rule_declarationContext *_localctx = _tracker.createInstance<Rule_declarationContext>(_ctx, getState());
  enterRule(_localctx, 4, extended_prologParser::RuleRule_declaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(43);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == extended_prologParser::NUMBER) {
      setState(41);
      match(extended_prologParser::NUMBER);
      setState(42);
      match(extended_prologParser::T__0);
    }
    setState(45);
    head_expr();
    setState(46);
    match(extended_prologParser::T__1);
    setState(47);
    body_elements();
    setState(48);
    match(extended_prologParser::T__2);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- WorldContext ------------------------------------------------------------------

extended_prologParser::WorldContext::WorldContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* extended_prologParser::WorldContext::WORLD_NAME() {
  return getToken(extended_prologParser::WORLD_NAME, 0);
}

std::vector<extended_prologParser::Fact_declarationContext *> extended_prologParser::WorldContext::fact_declaration() {
  return getRuleContexts<extended_prologParser::Fact_declarationContext>();
}

extended_prologParser::Fact_declarationContext* extended_prologParser::WorldContext::fact_declaration(size_t i) {
  return getRuleContext<extended_prologParser::Fact_declarationContext>(i);
}


size_t extended_prologParser::WorldContext::getRuleIndex() const {
  return extended_prologParser::RuleWorld;
}

void extended_prologParser::WorldContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterWorld(this);
}

void extended_prologParser::WorldContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitWorld(this);
}


std::any extended_prologParser::WorldContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitWorld(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::WorldContext* extended_prologParser::world() {
  WorldContext *_localctx = _tracker.createInstance<WorldContext>(_ctx, getState());
  enterRule(_localctx, 6, extended_prologParser::RuleWorld);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(50);
    match(extended_prologParser::WORLD_NAME);
    setState(54);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 671744) != 0)) {
      setState(51);
      fact_declaration();
      setState(56);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(57);
    match(extended_prologParser::T__2);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Fact_declarationContext ------------------------------------------------------------------

extended_prologParser::Fact_declarationContext::Fact_declarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

extended_prologParser::Fact_exprContext* extended_prologParser::Fact_declarationContext::fact_expr() {
  return getRuleContext<extended_prologParser::Fact_exprContext>(0);
}

tree::TerminalNode* extended_prologParser::Fact_declarationContext::NUMBER() {
  return getToken(extended_prologParser::NUMBER, 0);
}


size_t extended_prologParser::Fact_declarationContext::getRuleIndex() const {
  return extended_prologParser::RuleFact_declaration;
}

void extended_prologParser::Fact_declarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFact_declaration(this);
}

void extended_prologParser::Fact_declarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFact_declaration(this);
}


std::any extended_prologParser::Fact_declarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitFact_declaration(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Fact_declarationContext* extended_prologParser::fact_declaration() {
  Fact_declarationContext *_localctx = _tracker.createInstance<Fact_declarationContext>(_ctx, getState());
  enterRule(_localctx, 8, extended_prologParser::RuleFact_declaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(61);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == extended_prologParser::NUMBER) {
      setState(59);
      match(extended_prologParser::NUMBER);
      setState(60);
      match(extended_prologParser::T__0);
    }
    setState(63);
    fact_expr();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Head_exprContext ------------------------------------------------------------------

extended_prologParser::Head_exprContext::Head_exprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

extended_prologParser::Integrity_constraintContext* extended_prologParser::Head_exprContext::integrity_constraint() {
  return getRuleContext<extended_prologParser::Integrity_constraintContext>(0);
}

extended_prologParser::Fact_exprContext* extended_prologParser::Head_exprContext::fact_expr() {
  return getRuleContext<extended_prologParser::Fact_exprContext>(0);
}


size_t extended_prologParser::Head_exprContext::getRuleIndex() const {
  return extended_prologParser::RuleHead_expr;
}

void extended_prologParser::Head_exprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterHead_expr(this);
}

void extended_prologParser::Head_exprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitHead_expr(this);
}


std::any extended_prologParser::Head_exprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitHead_expr(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Head_exprContext* extended_prologParser::head_expr() {
  Head_exprContext *_localctx = _tracker.createInstance<Head_exprContext>(_ctx, getState());
  enterRule(_localctx, 10, extended_prologParser::RuleHead_expr);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(67);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case extended_prologParser::T__3:
      case extended_prologParser::T__4:
      case extended_prologParser::T__5:
      case extended_prologParser::T__6: {
        enterOuterAlt(_localctx, 1);
        setState(65);
        integrity_constraint();
        break;
      }

      case extended_prologParser::NEGATION_SIGN:
      case extended_prologParser::PREDICATE_NAME: {
        enterOuterAlt(_localctx, 2);
        setState(66);
        fact_expr();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Integrity_constraintContext ------------------------------------------------------------------

extended_prologParser::Integrity_constraintContext::Integrity_constraintContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t extended_prologParser::Integrity_constraintContext::getRuleIndex() const {
  return extended_prologParser::RuleIntegrity_constraint;
}

void extended_prologParser::Integrity_constraintContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIntegrity_constraint(this);
}

void extended_prologParser::Integrity_constraintContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIntegrity_constraint(this);
}


std::any extended_prologParser::Integrity_constraintContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitIntegrity_constraint(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Integrity_constraintContext* extended_prologParser::integrity_constraint() {
  Integrity_constraintContext *_localctx = _tracker.createInstance<Integrity_constraintContext>(_ctx, getState());
  enterRule(_localctx, 12, extended_prologParser::RuleIntegrity_constraint);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(69);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 240) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Fact_exprContext ------------------------------------------------------------------

extended_prologParser::Fact_exprContext::Fact_exprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

extended_prologParser::AtomContext* extended_prologParser::Fact_exprContext::atom() {
  return getRuleContext<extended_prologParser::AtomContext>(0);
}

tree::TerminalNode* extended_prologParser::Fact_exprContext::NEGATION_SIGN() {
  return getToken(extended_prologParser::NEGATION_SIGN, 0);
}


size_t extended_prologParser::Fact_exprContext::getRuleIndex() const {
  return extended_prologParser::RuleFact_expr;
}

void extended_prologParser::Fact_exprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFact_expr(this);
}

void extended_prologParser::Fact_exprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFact_expr(this);
}


std::any extended_prologParser::Fact_exprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitFact_expr(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Fact_exprContext* extended_prologParser::fact_expr() {
  Fact_exprContext *_localctx = _tracker.createInstance<Fact_exprContext>(_ctx, getState());
  enterRule(_localctx, 14, extended_prologParser::RuleFact_expr);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(72);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == extended_prologParser::NEGATION_SIGN) {
      setState(71);
      match(extended_prologParser::NEGATION_SIGN);
    }
    setState(74);
    atom();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Body_elementsContext ------------------------------------------------------------------

extended_prologParser::Body_elementsContext::Body_elementsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<extended_prologParser::Body_elementContext *> extended_prologParser::Body_elementsContext::body_element() {
  return getRuleContexts<extended_prologParser::Body_elementContext>();
}

extended_prologParser::Body_elementContext* extended_prologParser::Body_elementsContext::body_element(size_t i) {
  return getRuleContext<extended_prologParser::Body_elementContext>(i);
}


size_t extended_prologParser::Body_elementsContext::getRuleIndex() const {
  return extended_prologParser::RuleBody_elements;
}

void extended_prologParser::Body_elementsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBody_elements(this);
}

void extended_prologParser::Body_elementsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBody_elements(this);
}


std::any extended_prologParser::Body_elementsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitBody_elements(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Body_elementsContext* extended_prologParser::body_elements() {
  Body_elementsContext *_localctx = _tracker.createInstance<Body_elementsContext>(_ctx, getState());
  enterRule(_localctx, 16, extended_prologParser::RuleBody_elements);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(76);
    body_element();
    setState(81);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == extended_prologParser::T__7) {
      setState(77);
      match(extended_prologParser::T__7);
      setState(78);
      body_element();
      setState(83);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Body_elementContext ------------------------------------------------------------------

extended_prologParser::Body_elementContext::Body_elementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

extended_prologParser::Literal_exprContext* extended_prologParser::Body_elementContext::literal_expr() {
  return getRuleContext<extended_prologParser::Literal_exprContext>(0);
}

extended_prologParser::Constraint_exprContext* extended_prologParser::Body_elementContext::constraint_expr() {
  return getRuleContext<extended_prologParser::Constraint_exprContext>(0);
}


size_t extended_prologParser::Body_elementContext::getRuleIndex() const {
  return extended_prologParser::RuleBody_element;
}

void extended_prologParser::Body_elementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBody_element(this);
}

void extended_prologParser::Body_elementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBody_element(this);
}


std::any extended_prologParser::Body_elementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitBody_element(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Body_elementContext* extended_prologParser::body_element() {
  Body_elementContext *_localctx = _tracker.createInstance<Body_elementContext>(_ctx, getState());
  enterRule(_localctx, 18, extended_prologParser::RuleBody_element);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(86);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case extended_prologParser::WEAK_NEGATION_SIGN:
      case extended_prologParser::NEGATION_SIGN:
      case extended_prologParser::PREDICATE_NAME: {
        enterOuterAlt(_localctx, 1);
        setState(84);
        literal_expr();
        break;
      }

      case extended_prologParser::VARIABLE:
      case extended_prologParser::CONSTANT: {
        enterOuterAlt(_localctx, 2);
        setState(85);
        constraint_expr();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Literal_exprContext ------------------------------------------------------------------

extended_prologParser::Literal_exprContext::Literal_exprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

extended_prologParser::AtomContext* extended_prologParser::Literal_exprContext::atom() {
  return getRuleContext<extended_prologParser::AtomContext>(0);
}

tree::TerminalNode* extended_prologParser::Literal_exprContext::WEAK_NEGATION_SIGN() {
  return getToken(extended_prologParser::WEAK_NEGATION_SIGN, 0);
}

tree::TerminalNode* extended_prologParser::Literal_exprContext::NEGATION_SIGN() {
  return getToken(extended_prologParser::NEGATION_SIGN, 0);
}


size_t extended_prologParser::Literal_exprContext::getRuleIndex() const {
  return extended_prologParser::RuleLiteral_expr;
}

void extended_prologParser::Literal_exprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLiteral_expr(this);
}

void extended_prologParser::Literal_exprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLiteral_expr(this);
}


std::any extended_prologParser::Literal_exprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitLiteral_expr(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Literal_exprContext* extended_prologParser::literal_expr() {
  Literal_exprContext *_localctx = _tracker.createInstance<Literal_exprContext>(_ctx, getState());
  enterRule(_localctx, 20, extended_prologParser::RuleLiteral_expr);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(89);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == extended_prologParser::WEAK_NEGATION_SIGN) {
      setState(88);
      match(extended_prologParser::WEAK_NEGATION_SIGN);
    }
    setState(92);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == extended_prologParser::NEGATION_SIGN) {
      setState(91);
      match(extended_prologParser::NEGATION_SIGN);
    }
    setState(94);
    atom();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Constraint_exprContext ------------------------------------------------------------------

extended_prologParser::Constraint_exprContext::Constraint_exprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t extended_prologParser::Constraint_exprContext::getRuleIndex() const {
  return extended_prologParser::RuleConstraint_expr;
}

void extended_prologParser::Constraint_exprContext::copyFrom(Constraint_exprContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- StrEqualityConstraintContext ------------------------------------------------------------------

std::vector<extended_prologParser::Term_exprContext *> extended_prologParser::StrEqualityConstraintContext::term_expr() {
  return getRuleContexts<extended_prologParser::Term_exprContext>();
}

extended_prologParser::Term_exprContext* extended_prologParser::StrEqualityConstraintContext::term_expr(size_t i) {
  return getRuleContext<extended_prologParser::Term_exprContext>(i);
}

extended_prologParser::Operator_signContext* extended_prologParser::StrEqualityConstraintContext::operator_sign() {
  return getRuleContext<extended_prologParser::Operator_signContext>(0);
}

extended_prologParser::StrEqualityConstraintContext::StrEqualityConstraintContext(Constraint_exprContext *ctx) { copyFrom(ctx); }

void extended_prologParser::StrEqualityConstraintContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStrEqualityConstraint(this);
}
void extended_prologParser::StrEqualityConstraintContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStrEqualityConstraint(this);
}

std::any extended_prologParser::StrEqualityConstraintContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitStrEqualityConstraint(this);
  else
    return visitor->visitChildren(this);
}
extended_prologParser::Constraint_exprContext* extended_prologParser::constraint_expr() {
  Constraint_exprContext *_localctx = _tracker.createInstance<Constraint_exprContext>(_ctx, getState());
  enterRule(_localctx, 22, extended_prologParser::RuleConstraint_expr);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<extended_prologParser::StrEqualityConstraintContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(96);
    term_expr();
    setState(97);
    operator_sign();
    setState(98);
    term_expr();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Term_exprContext ------------------------------------------------------------------

extended_prologParser::Term_exprContext::Term_exprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t extended_prologParser::Term_exprContext::getRuleIndex() const {
  return extended_prologParser::RuleTerm_expr;
}

void extended_prologParser::Term_exprContext::copyFrom(Term_exprContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ArgTypeCONSTANTContext ------------------------------------------------------------------

tree::TerminalNode* extended_prologParser::ArgTypeCONSTANTContext::CONSTANT() {
  return getToken(extended_prologParser::CONSTANT, 0);
}

extended_prologParser::ArgTypeCONSTANTContext::ArgTypeCONSTANTContext(Term_exprContext *ctx) { copyFrom(ctx); }

void extended_prologParser::ArgTypeCONSTANTContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArgTypeCONSTANT(this);
}
void extended_prologParser::ArgTypeCONSTANTContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArgTypeCONSTANT(this);
}

std::any extended_prologParser::ArgTypeCONSTANTContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitArgTypeCONSTANT(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ArgTypeVARIABLEContext ------------------------------------------------------------------

tree::TerminalNode* extended_prologParser::ArgTypeVARIABLEContext::VARIABLE() {
  return getToken(extended_prologParser::VARIABLE, 0);
}

extended_prologParser::ArgTypeVARIABLEContext::ArgTypeVARIABLEContext(Term_exprContext *ctx) { copyFrom(ctx); }

void extended_prologParser::ArgTypeVARIABLEContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArgTypeVARIABLE(this);
}
void extended_prologParser::ArgTypeVARIABLEContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArgTypeVARIABLE(this);
}

std::any extended_prologParser::ArgTypeVARIABLEContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitArgTypeVARIABLE(this);
  else
    return visitor->visitChildren(this);
}
extended_prologParser::Term_exprContext* extended_prologParser::term_expr() {
  Term_exprContext *_localctx = _tracker.createInstance<Term_exprContext>(_ctx, getState());
  enterRule(_localctx, 24, extended_prologParser::RuleTerm_expr);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(102);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case extended_prologParser::VARIABLE: {
        _localctx = _tracker.createInstance<extended_prologParser::ArgTypeVARIABLEContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(100);
        match(extended_prologParser::VARIABLE);
        break;
      }

      case extended_prologParser::CONSTANT: {
        _localctx = _tracker.createInstance<extended_prologParser::ArgTypeCONSTANTContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(101);
        match(extended_prologParser::CONSTANT);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Operator_signContext ------------------------------------------------------------------

extended_prologParser::Operator_signContext::Operator_signContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t extended_prologParser::Operator_signContext::getRuleIndex() const {
  return extended_prologParser::RuleOperator_sign;
}

void extended_prologParser::Operator_signContext::copyFrom(Operator_signContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- Eq_predicateContext ------------------------------------------------------------------

extended_prologParser::Eq_predicateContext::Eq_predicateContext(Operator_signContext *ctx) { copyFrom(ctx); }

void extended_prologParser::Eq_predicateContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterEq_predicate(this);
}
void extended_prologParser::Eq_predicateContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitEq_predicate(this);
}

std::any extended_prologParser::Eq_predicateContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitEq_predicate(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Neq_predicateContext ------------------------------------------------------------------

extended_prologParser::Neq_predicateContext::Neq_predicateContext(Operator_signContext *ctx) { copyFrom(ctx); }

void extended_prologParser::Neq_predicateContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNeq_predicate(this);
}
void extended_prologParser::Neq_predicateContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNeq_predicate(this);
}

std::any extended_prologParser::Neq_predicateContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitNeq_predicate(this);
  else
    return visitor->visitChildren(this);
}
extended_prologParser::Operator_signContext* extended_prologParser::operator_sign() {
  Operator_signContext *_localctx = _tracker.createInstance<Operator_signContext>(_ctx, getState());
  enterRule(_localctx, 26, extended_prologParser::RuleOperator_sign);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(106);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case extended_prologParser::T__8: {
        _localctx = _tracker.createInstance<extended_prologParser::Eq_predicateContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(104);
        match(extended_prologParser::T__8);
        break;
      }

      case extended_prologParser::T__9: {
        _localctx = _tracker.createInstance<extended_prologParser::Neq_predicateContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(105);
        match(extended_prologParser::T__9);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AtomContext ------------------------------------------------------------------

extended_prologParser::AtomContext::AtomContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* extended_prologParser::AtomContext::PREDICATE_NAME() {
  return getToken(extended_prologParser::PREDICATE_NAME, 0);
}

extended_prologParser::Arguments_listContext* extended_prologParser::AtomContext::arguments_list() {
  return getRuleContext<extended_prologParser::Arguments_listContext>(0);
}


size_t extended_prologParser::AtomContext::getRuleIndex() const {
  return extended_prologParser::RuleAtom;
}

void extended_prologParser::AtomContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAtom(this);
}

void extended_prologParser::AtomContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAtom(this);
}


std::any extended_prologParser::AtomContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitAtom(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::AtomContext* extended_prologParser::atom() {
  AtomContext *_localctx = _tracker.createInstance<AtomContext>(_ctx, getState());
  enterRule(_localctx, 28, extended_prologParser::RuleAtom);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(108);
    match(extended_prologParser::PREDICATE_NAME);
    setState(113);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == extended_prologParser::T__10) {
      setState(109);
      match(extended_prologParser::T__10);
      setState(110);
      arguments_list();
      setState(111);
      match(extended_prologParser::T__11);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Arguments_listContext ------------------------------------------------------------------

extended_prologParser::Arguments_listContext::Arguments_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<extended_prologParser::Term_exprContext *> extended_prologParser::Arguments_listContext::term_expr() {
  return getRuleContexts<extended_prologParser::Term_exprContext>();
}

extended_prologParser::Term_exprContext* extended_prologParser::Arguments_listContext::term_expr(size_t i) {
  return getRuleContext<extended_prologParser::Term_exprContext>(i);
}


size_t extended_prologParser::Arguments_listContext::getRuleIndex() const {
  return extended_prologParser::RuleArguments_list;
}

void extended_prologParser::Arguments_listContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArguments_list(this);
}

void extended_prologParser::Arguments_listContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<extended_prologListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArguments_list(this);
}


std::any extended_prologParser::Arguments_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<extended_prologVisitor*>(visitor))
    return parserVisitor->visitArguments_list(this);
  else
    return visitor->visitChildren(this);
}

extended_prologParser::Arguments_listContext* extended_prologParser::arguments_list() {
  Arguments_listContext *_localctx = _tracker.createInstance<Arguments_listContext>(_ctx, getState());
  enterRule(_localctx, 30, extended_prologParser::RuleArguments_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(115);
    term_expr();
    setState(120);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == extended_prologParser::T__7) {
      setState(116);
      match(extended_prologParser::T__7);
      setState(117);
      term_expr();
      setState(122);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void extended_prologParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  extended_prologParserInitialize();
#else
  ::antlr4::internal::call_once(extended_prologParserOnceFlag, extended_prologParserInitialize);
#endif
}
