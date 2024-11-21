#ifndef CECS_BITSET_H
#define CECS_BITSET_H

#include <stdint.h>
#include <intrin.h>
#include "cecs_dynamic_array.h"
#include "cecs_range.h"
#include "cecs_union.h"

#define CECS_BIT_PAGE_SIZE_LOG2 2
#define CECS_BIT_PAGE_SIZE (1 << CECS_BIT_PAGE_SIZE_LOG2)


#if (SIZE_MAX == 0xFFFF)
#define CECS_BIT_WORD_BITS_LOG2 4 

#elif (SIZE_MAX == 0xFFFFFFFF)
#define CECS_BIT_WORD_BITS_LOG2 5

#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
#define CECS_BIT_WORD_BITS_LOG2 6

#else
    #error TBD code SIZE_T_BITS

#endif
#define CECS_BIT_WORD_BITS (1 << CECS_BIT_WORD_BITS_LOG2)
#define CECS_BIT_WORD_TYPE size_t

typedef CECS_BIT_WORD_TYPE cecs_bit_word;
#define CECS_BIT_WORD_BIT_COUNT (8 * sizeof(cecs_bit_word))
static_assert(CECS_BIT_WORD_BITS == CECS_BIT_WORD_BIT_COUNT, "CECS_BIT_WORD_BITS != CECS_BIT_WORD_BIT_COUNT");

#define CECS_BIT_LAYER_COUNT (CECS_BIT_WORD_BITS_LOG2 - CECS_BIT_PAGE_SIZE_LOG2)

#define CECS_ZERO_PAGE_MASK (((cecs_bit_word)1 << CECS_BIT_PAGE_SIZE) - 1)
inline cecs_bit_word cecs_page_mask(size_t page_index) {
    return ((cecs_bit_word)CECS_ZERO_PAGE_MASK << (page_index * CECS_BIT_PAGE_SIZE));
}

inline size_t cecs_layer_word_index(size_t bit_index, size_t layer) { 
    return bit_index >> (CECS_BIT_WORD_BITS_LOG2 + layer * CECS_BIT_PAGE_SIZE_LOG2);
}

inline size_t cecs_bit0_from_layer_word_index(size_t word_index, size_t layer) {
    return word_index << (CECS_BIT_WORD_BITS_LOG2 + layer * CECS_BIT_PAGE_SIZE_LOG2);
}

inline size_t cecs_layer_bit_index(size_t bit_index, size_t layer) {
    return bit_index >> (layer * CECS_BIT_PAGE_SIZE_LOG2);
}

inline size_t cecs_bit0_from_layer_bit_index(size_t layer_bit_index, size_t layer) {
    return layer_bit_index << (layer * CECS_BIT_PAGE_SIZE_LOG2);
}

inline size_t cecs_layer_word_bit_index(size_t bit_index, size_t layer) {
    return cecs_layer_bit_index(bit_index, layer) & (CECS_BIT_WORD_BIT_COUNT - 1);
}

typedef cecs_exclusive_range cecs_word_range;

typedef struct cecs_bitset {
    cecs_dynamic_array bit_words;
    cecs_word_range word_range;
} cecs_bitset;

cecs_bitset cecs_bitset_create(cecs_arena *a, size_t capacity);

void cecs_bitset_unset_all(cecs_bitset *b);

cecs_word_range cecs_bitset_expand(cecs_bitset *b, cecs_arena *a, size_t word_index);

cecs_bit_word cecs_bitset_set(cecs_bitset *b, cecs_arena *a, size_t bit_index);

cecs_bit_word cecs_bitset_unset(cecs_bitset *b, cecs_arena *a, size_t bit_index);

cecs_bit_word cecs_bitset_get_word(const cecs_bitset *b, size_t bit_index);

bool cecs_bitset_is_set(const cecs_bitset *b, size_t bit_index);

bool cecs_bitset_bit_in_range(const cecs_bitset *b, size_t bit_index);


typedef struct cecs_bitset_iterator {
    const cecs_bitset *const bitset;
    size_t current_bit_index;
} cecs_bitset_iterator;

cecs_bitset_iterator cecs_bitset_iterator_create(const cecs_bitset *b);;

inline bool cecs_bitset_iterator_done(const cecs_bitset_iterator *it) {
    return !cecs_bitset_bit_in_range(it->bitset, it->current_bit_index);
}

inline size_t cecs_bitset_iterator_next(cecs_bitset_iterator *it) {
    return ++it->current_bit_index;
}

size_t cecs_bitset_iterator_next_set(cecs_bitset_iterator *it);

size_t cecs_bitset_iterator_next_unset(cecs_bitset_iterator *it);

inline size_t cecs_bitset_iterator_current(const cecs_bitset_iterator *it) {
    return it->current_bit_index;
}

inline bool cecs_bitset_iterator_current_is_set(const cecs_bitset_iterator *it) {
    return cecs_bitset_is_set(it->bitset, cecs_bitset_iterator_current(it));
}


typedef struct cecs_hibitset {
    cecs_bitset bitsets[CECS_BIT_LAYER_COUNT];
} cecs_hibitset;

inline const cecs_hibitset cecs_hibitset_empty(void) {
    return (cecs_hibitset){0};
}

inline size_t cecs_layer_complement(size_t layer) {
    return CECS_BIT_LAYER_COUNT - layer - 1;
}

cecs_hibitset cecs_hibitset_create(cecs_arena *a);

void cecs_hibitset_unset_all(cecs_hibitset *b);

void cecs_hibitset_set(cecs_hibitset *b, cecs_arena *a, size_t bit_index);

void cecs_hibitset_unset(cecs_hibitset *b, cecs_arena *a, size_t bit_index);

bool cecs_hibitset_is_set(const cecs_hibitset *b, size_t bit_index);

#pragma intrinsic(_BitScanForward)
bool cecs_hibitset_is_set_skip_unset(const cecs_hibitset *b, size_t bit_index, cecs_ssize_t *out_unset_bit_skip_count);

#pragma intrinsic(_BitScanReverse)
bool cecs_hibitset_is_set_skip_unset_reverse(const cecs_hibitset *b, size_t bit_index, cecs_ssize_t *out_unset_bit_skip_count);


cecs_bit_word cecs_hibitset_get_word(const cecs_hibitset *b, size_t bit_index);

cecs_exclusive_range cecs_hibitset_bit_range(const cecs_hibitset *b);

bool cecs_hibitset_bit_in_range(const cecs_hibitset *b, size_t bit_index);


typedef struct cecs_hibitset_iterator {
    const CECS_COW_STRUCT(const cecs_hibitset, cecs_hibitset) hibitset;
    size_t current_bit_index;
} cecs_hibitset_iterator;

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at(const cecs_hibitset *b, size_t bit_index);

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at_first(const cecs_hibitset *b);

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at_last(const cecs_hibitset *b);

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at(const cecs_hibitset b, size_t bit_index);

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at_first(const cecs_hibitset b);

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at_last(const cecs_hibitset b);

inline bool cecs_hibitset_iterator_done(const cecs_hibitset_iterator *it) {
    return !cecs_hibitset_bit_in_range(CECS_COW_GET_REFERENCE(cecs_hibitset, it->hibitset), it->current_bit_index);
}

inline size_t cecs_hibitset_iterator_next(cecs_hibitset_iterator *it) {
    return ++it->current_bit_index;
}

inline size_t cecs_hibitset_iterator_previous(cecs_hibitset_iterator *it) {
    return --it->current_bit_index;
}

size_t cecs_hibitset_iterator_next_set(cecs_hibitset_iterator *it);

size_t cecs_hibitset_iterator_previous_set(cecs_hibitset_iterator *it);

inline size_t cecs_hibitset_iterator_current(const cecs_hibitset_iterator *it) {
    return it->current_bit_index;
}

inline bool cecs_hibitset_iterator_current_is_set(const cecs_hibitset_iterator *it) {
    return cecs_hibitset_is_set(CECS_COW_GET_REFERENCE(cecs_hibitset, it->hibitset), cecs_hibitset_iterator_current(it));
}


// TODO: test hibitset operations onto, result mutates parameter
cecs_hibitset cecs_hibitset_intersection(const cecs_hibitset *bitsets, size_t count, cecs_arena *a);

cecs_hibitset cecs_hibitset_union(const cecs_hibitset *bitsets, size_t count, cecs_arena *a);

cecs_hibitset cecs_hibitset_difference(const cecs_hibitset *bitset, const cecs_hibitset *subtracted_bitsets, size_t count, cecs_arena *a);


cecs_hibitset *cecs_hibitset_intersect(cecs_hibitset *self, const cecs_hibitset *bitsets, size_t count, cecs_arena *a);

cecs_hibitset *cecs_hibitset_join(cecs_hibitset *self, const cecs_hibitset *bitsets, size_t count, cecs_arena *a);

cecs_hibitset *cecs_hibitset_subtract(cecs_hibitset *self, const cecs_hibitset *subtracted_bitsets, size_t count, cecs_arena *a);


#endif