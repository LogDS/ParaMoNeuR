//
// Created by gyankos on 23/10/26.
//

#ifndef PARAMONEUR2_CANONICAL_FORM_H
#define PARAMONEUR2_CANONICAL_FORM_H
#include "logds/paramoneur/language/compiled_structures.h"


/*
 * Formal reference:
 *   De Bruijn, N. G. (1972). Lambda calculus notation with nameless dummies, a tool for
 *   automatic formula manipulation, with application to the Church-Rosser theorem.
 *   Indagationes Mathematicae (Proceedings), 75(5), 381-392.
 *
 *   Inspiration: The De Bruijn index replaces variable names (X, Y) with integers that
 *   indicate the positional distance from the respective quantifier. Your function applies
 *   the same semantic principle in a relational form: by renaming the variables to V0, V1,
 *   V2 strictly based on the left-to-right positional order of appearance, it removes the
 *   arbitrariness of the names chosen by the user and maps the rule onto a structurally
 *   rigid canonical string.
 *
 *   The efficient implementation based on Disjoint-Set (Union-Find) data structures to
 *   partition variables and unify structural equality constraints with quasi-linear
 *   complexity:
 *
 *   Formal reference (Union-Find): Tarjan, R. E. (1975). Efficiency of a good but not
 *   linear set union algorithm. Journal of the ACM (JACM), 22(2), 215-225.
 *   Applied reference (Logic/Resolution): Huet, G. (1976). Resolution d'équations dans des
 *   langages d'ordre 1, 2, ..., ω. Thèse de doctorat d'État, Université Paris VII.
 *
 *   Inspiration: Gérard Huet showed that to test the structural equivalence of terms and
 *   logical equations efficiently, the use of Union-Find makes it possible to collapse the
 *   equivalence classes of variable bindings without explicitly generating an infinite
 *   number of substitutions. In your function, the Union-Find algorithm tracks the
 *   accumulated equality constraints (constraints), ensuring that if X == Y, they are
 *   merged into the same canonical class and receive the same identical final numeric
 *   identifier (V_i), locking in the formal isomorphism.
 *
 *   Baader, F., & Nipkow, T. (1998). Term Rewriting and All That. Cambridge University Press.
 *
 *   In rewriting systems, two terms are said to be in canonical α-form if their normalized
 *   textual representation is identical. The ordering of the tail literals implemented in
 *   the framework follows the Lexicographical Path Ordering (LPO) techniques described in
 *   this text. By ordering the literals according to predicate name, arity and sign after
 *   renaming, the algorithm guarantees that
 *
 *   target(Y) :- p(X,Y), s(Y). and target(B) :- s(B), p(A,B). (written with inverted bodies)
 *
 *   collapse into the same identical canonical textual dump, offering the unification
 *   module of Step 3 and the trainer of Step 5 a normalized input free of combinatorial
 *   redundancies.
 *
 *   :param rule:
 *   :return:
 */
logds::paramoneur::UltimateHornRule getCanonicalAlphaForm(const logds::paramoneur::UltimateHornRule& rule);

std::vector<int64_t> prune_equivalent_rules_before_tensor_idx(const std::vector<logds::paramoneur::UltimateHornRule>& rule_list);

#endif //PARAMONEUR2_CANONICAL_FORM_H
