#ifndef BITSET_H
#define BITSET_H

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <intrin.h>
#include "list.h"
#include "range.h"

#define BIT_PAGE_SIZE_LOG2 2
#define BIT_PAGE_SIZE (1 << BIT_PAGE_SIZE_LOG2)

#define BIT_WORD_BITS_LOG2 6
#define BIT_WORD_BITS (1 << BIT_WORD_BITS_LOG2)
#define BIT_WORD_TYPE uint64_t

typedef BIT_WORD_TYPE bit_word;
#define BIT_WORD_BIT_COUNT (8 * sizeof(bit_word))
static_assert(BIT_WORD_BITS == BIT_WORD_BIT_COUNT, "BIT_WORD_BITS != BIT_WORD_BIT_COUNT");

#define BIT_LAYER_COUNT (BIT_WORD_BITS_LOG2 - BIT_PAGE_SIZE_LOG2)

#define ZERO_PAGE_MASK (((bit_word)1 << BIT_PAGE_SIZE) - 1)
inline bit_word page_mask(size_t page_index) {
    return ((bit_word)ZERO_PAGE_MASK << (page_index * BIT_PAGE_SIZE));
}

inline size_t layer_word_index(size_t bit_index, size_t layer) { 
    return bit_index >> (BIT_WORD_BITS_LOG2 + layer * BIT_PAGE_SIZE_LOG2);
}

inline size_t bit0_from_layer_word_index(size_t word_index, size_t layer) {
    return word_index << (BIT_WORD_BITS_LOG2 + layer * BIT_PAGE_SIZE_LOG2);
}

inline size_t layer_bit_index(size_t bit_index, size_t layer) {
    return bit_index >> (layer * BIT_PAGE_SIZE_LOG2);
}

inline size_t bit0_from_layer_bit_index(size_t layer_bit_index, size_t layer) {
    return layer_bit_index << (layer * BIT_PAGE_SIZE_LOG2);
}

inline size_t layer_word_bit_index(size_t bit_index, size_t layer) {
    return layer_bit_index(bit_index, layer) & (BIT_WORD_BIT_COUNT - 1);
}

typedef exclusive_range word_range;

typedef struct bitset {
    list bit_words;
    word_range word_range;
} bitset;

bitset bitset_create(arena *a, size_t capacity);

void bitset_unset_all(bitset *b);

word_range bitset_expand(bitset *b, arena *a, size_t word_index);

bit_word bitset_set(bitset *b, arena *a, size_t bit_index);

bit_word bitset_unset(bitset *b, arena *a, size_t bit_index);

bit_word bitset_get_word(const bitset *b, size_t bit_index);

bool bitset_is_set(const bitset *b, size_t bit_index);

bool bitset_bit_in_range(const bitset *b, size_t bit_index);



typedef struct bitset_iterator {
    const bitset *const bitset;
    size_t current_bit_index;
} bitset_iterator;

bitset_iterator bitset_iterator_create(const bitset *b);;

inline bool bitset_iterator_done(const bitset_iterator *it) {
    return !bitset_bit_in_range(it->bitset, it->current_bit_index);
}

inline size_t bitset_iterator_next(bitset_iterator *it) {
    return ++it->current_bit_index;
}

size_t bitset_iterator_next_set(bitset_iterator *it);

size_t bitset_iterator_next_unset(bitset_iterator *it);

inline size_t bitset_iterator_current(const bitset_iterator *it) {
    return it->current_bit_index;
}

inline bool bitset_iterator_current_is_set(const bitset_iterator *it) {
    return bitset_is_set(it->bitset, bitset_iterator_current(it));
}


typedef struct hibitset {
    bitset bitsets[BIT_LAYER_COUNT];
} hibitset;

inline const hibitset hibitset_empty() {
    return (hibitset){0};
}

inline size_t layer_complement(size_t layer) {
    return BIT_LAYER_COUNT - layer - 1;
}

hibitset hibitset_create(arena *a);

void hibitset_unset_all(hibitset *b);

void hibitset_set(hibitset *b, arena *a, size_t bit_index);

void hibitset_unset(hibitset *b, arena *a, size_t bit_index);

bool hibitset_is_set(const hibitset *b, size_t bit_index);

#pragma intrinsic(_BitScanForward)
bool hibitset_is_set_skip_unset(const hibitset *b, size_t bit_index, ssize_t *out_unset_bit_skip_count);

#pragma intrinsic(_BitScanReverse)
bool hibitset_is_set_skip_unset_reverse(const hibitset *b, size_t bit_index, ssize_t *out_unset_bit_skip_count);


bit_word hibitset_get_word(const hibitset *b, size_t bit_index);

exclusive_range hibitset_bit_range(const hibitset *b);

bool hibitset_bit_in_range(const hibitset *b, size_t bit_index);


#include "tagged_union.h"
typedef struct hibitset_iterator {
    const COW_STRUCT(const hibitset, hibitset) hibitset;
    size_t current_bit_index;
} hibitset_iterator;

hibitset_iterator hibitset_iterator_create_borrowed_at(const hibitset *b, size_t bit_index);

hibitset_iterator hibitset_iterator_create_borrowed_at_first(const hibitset *b);

hibitset_iterator hibitset_iterator_create_borrowed_at_last(const hibitset *b);

hibitset_iterator hibitset_iterator_create_owned_at(const hibitset b, size_t bit_index);

hibitset_iterator hibitset_iterator_create_owned_at_first(const hibitset b);

hibitset_iterator hibitset_iterator_create_owned_at_last(const hibitset b);

inline bool hibitset_iterator_done(const hibitset_iterator *it) {
    return !hibitset_bit_in_range(COW_GET_REFERENCE(hibitset, it->hibitset), it->current_bit_index);
}

inline size_t hibitset_iterator_next(hibitset_iterator *it) {
    return ++it->current_bit_index;
}

inline size_t hibitset_iterator_previous(hibitset_iterator *it) {
    return --it->current_bit_index;
}

size_t hibitset_iterator_next_set(hibitset_iterator *it);

size_t hibitset_iterator_previous_set(hibitset_iterator *it);

inline size_t hibitset_iterator_current(const hibitset_iterator *it) {
    return it->current_bit_index;
}

inline bool hibitset_iterator_current_is_set(const hibitset_iterator *it) {
    return hibitset_is_set(COW_GET_REFERENCE(hibitset, it->hibitset), hibitset_iterator_current(it));
}


// TODO: test hibitset operations onto, result mutates parameter
hibitset hibitset_intersection(const hibitset *bitsets, size_t count, arena *a);

hibitset hibitset_union(const hibitset *bitsets, size_t count, arena *a);

hibitset hibitset_difference(const hibitset *bitset, const hibitset *subtracted_bitsets, size_t count, arena *a);


hibitset *hibitset_intersect(hibitset *self, const hibitset *bitsets, size_t count, arena *a);

hibitset *hibitset_join(hibitset *self, const hibitset *bitsets, size_t count, arena *a);

hibitset *hibitset_subtract(hibitset *self, const hibitset *subtracted_bitsets, size_t count, arena *a);


#endif