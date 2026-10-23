//
// Created by gyankos on 23/10/26.
//

#ifndef PARAMONEUR2_INDEXING_H
#define PARAMONEUR2_INDEXING_H
#include <cstdint>

#include "logds/paramoneur/language/compiled_structures.h"

uint64_t compile_atom_to_index_param(
    const logds::paramoneur::ArbitraryAtom &atom,
    uint64_t b_val,
    uint64_t max_l_param,
    uint64_t stop_token);

#endif //PARAMONEUR2_INDEXING_H
