grammar extended_prolog;

program
    : line+
    ;

line
    : rule_declaration
    | world
    ;

rule_declaration
    : (NUMBER '::')? head_expr ':-' body_elements '.'
    ;

world: WORLD_NAME fact_declaration* '.' ;

fact_declaration
    : (NUMBER '::')? fact_expr
    ;

head_expr
    : integrity_constraint
    | fact_expr
    ;

integrity_constraint
    : 'false'
    | 'falsehood'
    | 'fail'
    | '⊥'
    | 'fail'
    ;

fact_expr
    : NEGATION_SIGN? atom
    ;

body_elements
    : body_element (',' body_element)*
    ;

body_element
    : literal_expr
    | constraint_expr
    ;

literal_expr
    : WEAK_NEGATION_SIGN? NEGATION_SIGN? atom
    ;

constraint_expr
    : term_expr operator_sign term_expr         # StrEqualityConstraint
    ;

term_expr
    : VARIABLE  #ArgTypeVARIABLE
    | CONSTANT  #ArgTypeCONSTANT
    ;

operator_sign
    : '=='       #eq_predicate
    | '!='       #neq_predicate
    ;

atom
    : PREDICATE_NAME ('(' arguments_list ')')?
    ;

arguments_list
    : term_expr (',' term_expr)*
    ;


// =============================================================================
// LEXER RULES (REGOLE LESSICALI)
// =============================================================================

// Operatori ed etichette di negazione esplicite
WEAK_NEGATION_SIGN : '~' ;
NEGATION_SIGN      : '¬' ;

// Convenzione Prolog: Variabili iniziano con Lettera Maiuscola o underscore
VARIABLE
    : [A-Z_][a-zA-Z0-9_]*
    ;

WORLD_NAME    : [a-z][0-9]+'.'
    ;

// Predicati e Costanti iniziano tipicamente con Lettera Minuscola o sono stringhe/numeri generici
PREDICATE_NAME
    : [a-z][a-zA-Z0-9_]*
    ;

CONSTANT
    : /*[a-z][a-zA-Z0-9_]*
    | NUMBER*/
    | '"' (~['"\r\n])* '"'
    ;

NUMBER
    : [0-9]+ ('.' [0-9]+)?
    ;


SPACE : [ \t\r\n]+ -> skip;

COMMENT
    : '/*' .*? '*/' -> skip
;

LINE_COMMENT
    : '//' ~[\r\n]* -> skip
;