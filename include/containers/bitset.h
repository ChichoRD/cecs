#ifndef BITSET_H
#define BITSET_H

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <intrin.h>
#include "list.h"
#include "range.h"
#include "view.h"

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

bitset bitset_create(arena *a, size_t capacity) {
    bitset b = (bitset){
        .bit_words = LIST_CREATE_WITH_CAPACITY(bit_word, a, capacity),
        .word_range = {0, 0},
    };
    //bit_word *buffer = calloc(capacity, sizeof(bit_word));
    //LIST_ADD_RANGE(bit_word, &b.bit_words, a, buffer, capacity);
    //free(buffer);
    return b;
}

void bitset_unset_all(bitset *b) {
    list_clear(&b->bit_words);
    b->word_range = (exclusive_range){0, 0};
}

word_range bitset_expand(bitset *b, arena *a, size_t word_index) {
    if (exclusive_range_is_empty(b->word_range)) {
        assert(LIST_COUNT(bit_word, &b->bit_words) == 0);
        LIST_ADD(bit_word, &b->bit_words, a, &((bit_word){0}));
        return (b->word_range = exclusive_range_singleton(word_index));
    }
    word_range expanded_range = exclusive_range_from(
        range_union(b->word_range.range, exclusive_range_singleton(word_index).range)
    );
    range_splitting expansion_ranges = exclusive_range_difference(
        expanded_range,
        b->word_range
    );
    
    word_range range0 = exclusive_range_from(expansion_ranges.ranges[0]);
    word_range range1 = exclusive_range_from(expansion_ranges.ranges[1]);
    if (!exclusive_range_is_empty(range0)) {
        size_t missing_count = exclusive_range_length(range0);
        bit_word *buffer = calloc(missing_count, sizeof(bit_word));
        LIST_INSERT_RANGE(bit_word, &b->bit_words, a, 0, buffer, missing_count);
        free(buffer);
    } else if (!exclusive_range_is_empty(range1)) {
        size_t missing_count = exclusive_range_length(range1);
        bit_word *buffer = calloc(missing_count, sizeof(bit_word));
        LIST_ADD_RANGE(bit_word, &b->bit_words, a, buffer, missing_count);
        free(buffer);
    }

    return (b->word_range = expanded_range);
}

bit_word bitset_set(bitset *b, arena *a, size_t bit_index) {
    size_t word_index = layer_word_index(bit_index, 0);
    if (!exclusive_range_contains(b->word_range, word_index)) {
        bitset_expand(b, a, word_index);
    }
    ssize_t list_index = (ssize_t)word_index - b->word_range.start;
    assert(
        list_index >= 0
        && list_index < exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );
    bit_word *w = LIST_GET(bit_word, &b->bit_words, list_index);
    return (*w |= (bit_word)1 << layer_word_bit_index(bit_index, 0));
}

bit_word bitset_unset(bitset *b, arena *a, size_t bit_index) {
    size_t word_index = layer_word_index(bit_index, 0);
    if (!exclusive_range_contains(b->word_range, word_index)) {
        bitset_expand(b, a, word_index);
    }
    ssize_t list_index = (ssize_t)word_index - b->word_range.start;
    assert(
        list_index >= 0
        && list_index < exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );
    bit_word *w = LIST_GET(bit_word, &b->bit_words, list_index);
    return (*w &= ~((bit_word)1 << layer_word_bit_index(bit_index, 0)));
}

bit_word bitset_get_word(const bitset *b, size_t bit_index) {
    size_t word_index = layer_word_index(bit_index, 0);
    if (!exclusive_range_contains(b->word_range, word_index)) {
        return (bit_word)0;
    }
    ssize_t list_index = (ssize_t)word_index - b->word_range.start;
    bit_word *w = LIST_GET(bit_word, &b->bit_words, list_index);
    return *w;
}

bool bitset_is_set(bitset *b, size_t bit_index) {
    return (bitset_get_word(b, bit_index)
        & ((bit_word)1 << layer_word_bit_index(bit_index, 0))) != 0;
}

bool bitset_bit_in_range(const bitset *b, size_t bit_index) {
    return exclusive_range_contains(b->word_range, layer_word_index(bit_index, 0));
}



typedef struct bitset_iterator {
    const bitset *const bitset;
    size_t current_bit_index;
} bitset_iterator;

bitset_iterator bitset_iterator_create(const bitset *b) {
    return (bitset_iterator){
        .bitset = b,
        .current_bit_index = bit0_from_layer_word_index(b->word_range.start, 0),
    };
};

inline bool bitset_iterator_done(const bitset_iterator *it) {
    return !bitset_bit_in_range(it->bitset, it->current_bit_index);
}

inline size_t bitset_iterator_next(bitset_iterator *it) {
    return ++it->current_bit_index;
}

size_t bitset_iterator_next_set(bitset_iterator *it) {
    do {
        bitset_iterator_next(it);
    } while (!bitset_iterator_done(it) && !bitset_is_set(it->bitset, it->current_bit_index));
    return it->current_bit_index;
}

size_t bitset_iterator_next_unset(bitset_iterator *it) {
    do {
        bitset_iterator_next(it);
    } while (!bitset_iterator_done(it) && bitset_is_set(it->bitset, it->current_bit_index));
    return it->current_bit_index;
}

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

hibitset hibitset_create(arena *a) {
    hibitset b;
    for (size_t layer = 0; layer < BIT_LAYER_COUNT; layer++) {
        b.bitsets[layer] = bitset_create(a, (size_t)1 << (BIT_PAGE_SIZE_LOG2 * layer_complement(layer)));
    }
    return b;
}

void hibitset_unset_all(hibitset *b) {
    for (size_t layer = 0; layer < BIT_LAYER_COUNT; layer++) {
        bitset_unset_all(&b->bitsets[layer]);
    }
}

void hibitset_set(hibitset *b, arena *a, size_t bit_index) {
    for (ssize_t layer = BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = layer_bit_index(bit_index, layer);
        bitset_set(&b->bitsets[layer], a, layer_bit);
    }
}

void hibitset_unset(hibitset *b, arena *a, size_t bit_index) {
    for (size_t layer = 0; layer < BIT_LAYER_COUNT; layer++) {
        size_t layer_bit = layer_bit_index(bit_index, layer);
        bit_word unset_word = bitset_unset(&b->bitsets[layer], a, layer_bit);

        size_t layer_word_bit_page = layer_word_bit_index(bit_index, layer) / BIT_PAGE_SIZE;
        bit_word unset_word_page = unset_word & page_mask(layer_word_bit_page);

        if (unset_word_page != 0) {
            break;
        }
    }
}

bool hibitset_is_set(const hibitset *b, size_t bit_index) {
    return bitset_is_set(&b->bitsets[0], bit_index);
}

#pragma intrinsic(_BitScanForward)
bool hibitset_is_set_skip_unset(const hibitset *b, size_t bit_index, ssize_t *out_unset_bit_skip_count) {
    *out_unset_bit_skip_count = 1;
    for (ssize_t layer = BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = layer_bit_index(bit_index, layer);
        bit_word word = bitset_get_word(&b->bitsets[layer], layer_bit);
        size_t layer_word_bit_shift = layer_bit & (BIT_WORD_BIT_COUNT - 1);
        bit_word word_shifted_to_bit = word >> layer_word_bit_shift;

        if ((word_shifted_to_bit & (bit_word)1) == (bit_word)0) {
            DWORD unset_low = 1;
            DWORD unset_high = 0;
            uint32_t word_low = (uint32_t)word_shifted_to_bit;
            uint32_t word_high = (uint32_t)(word_shifted_to_bit >> (BIT_WORD_BIT_COUNT >> 1));

            size_t unset_continuous_count = 0;
            if (!_BitScanForward(&unset_low, word_low)) {
                if (!_BitScanForward(&unset_high, word_high)) {
                    unset_continuous_count = BIT_WORD_BIT_COUNT - layer_word_bit_shift;
                } else {
                    unset_continuous_count = unset_high + (BIT_WORD_BIT_COUNT >> 1);
                }
            } else {
                unset_continuous_count = unset_low;
            }
            *out_unset_bit_skip_count =
                bit0_from_layer_bit_index(layer_bit + unset_continuous_count, layer) - bit_index;
            assert(*out_unset_bit_skip_count > 0);
            return false;
        }
    }
    return true;
}

#pragma intrinsic(_BitScanReverse)
bool hibitset_is_set_skip_unset_reverse(const hibitset *b, size_t bit_index, ssize_t *out_unset_bit_skip_count) {
    *out_unset_bit_skip_count = 1;
    for (ssize_t layer = BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = layer_bit_index(bit_index, layer);
        bit_word word = bitset_get_word(&b->bitsets[layer], layer_bit);
        size_t layer_word_bit_shift = layer_bit & (BIT_WORD_BIT_COUNT - 1);
        bit_word word_shifted_to_bit = word >> layer_word_bit_shift;

        if ((word_shifted_to_bit & (bit_word)1) == (bit_word)0) {
            size_t unset_low = 1;
            size_t unset_high = 0;
            uint32_t word_low = (uint32_t)word_shifted_to_bit;
            uint32_t word_high = (uint32_t)(word_shifted_to_bit >> (BIT_WORD_BIT_COUNT >> 1));

            size_t unset_continuous_count = 0;
            if (!_BitScanReverse(&unset_low, word_low)) {
                if (!_BitScanReverse(&unset_high, word_high)) {
                    unset_continuous_count = BIT_WORD_BIT_COUNT - layer_word_bit_shift;
                } else {
                    unset_continuous_count = unset_high + (BIT_WORD_BIT_COUNT >> 1);
                }
            } else {
                unset_continuous_count = unset_low;
            }
            *out_unset_bit_skip_count =
                bit_index - bit0_from_layer_bit_index(layer_bit - unset_continuous_count, layer);
            assert(*out_unset_bit_skip_count > 0);
            return false;
        }
    }
    return true;
}


bit_word hibitset_get_word(const hibitset *b, size_t bit_index) {
    return bitset_get_word(&b->bitsets[0], bit_index);
}

exclusive_range hibitset_bit_range(const hibitset *b) {
    return (exclusive_range){ 
        .start = bit0_from_layer_word_index(b->bitsets[0].word_range.start, 0),
        .end = bit0_from_layer_word_index(b->bitsets[0].word_range.end, 0),
    };
}

bool hibitset_bit_in_range(const hibitset *b, size_t bit_index) {
    return bitset_bit_in_range(&b->bitsets[0], bit_index);
}


#include "tagged_union.h"
typedef struct hibitset_iterator {
    const COW_STRUCT(const hibitset, hibitset) hibitset;
    size_t current_bit_index;
} hibitset_iterator;

hibitset_iterator hibitset_iterator_create_borrowed_at(const hibitset *b, size_t bit_index) {
    return (hibitset_iterator){
        .hibitset = COW_CREATE_BORROWED(hibitset, b),
        .current_bit_index = bit_index,
    };
}

hibitset_iterator hibitset_iterator_create_borrowed_at_first(const hibitset *b) {
    return (hibitset_iterator){
        .hibitset = COW_CREATE_BORROWED(hibitset, b),
        .current_bit_index = hibitset_bit_range(b).start,
    };
}

hibitset_iterator hibitset_iterator_create_borrowed_at_last(const hibitset *b) {
    return (hibitset_iterator){
        .hibitset = COW_CREATE_BORROWED(hibitset, b),
        .current_bit_index = hibitset_bit_range(b).end - 1,
    };
}

hibitset_iterator hibitset_iterator_create_owned_at(const hibitset b, size_t bit_index) {
    return (hibitset_iterator){
        .hibitset = COW_CREATE_OWNED(hibitset, b),
        .current_bit_index = bit_index,
    };
}

hibitset_iterator hibitset_iterator_create_owned_at_first(const hibitset b) {
    return (hibitset_iterator){
        .hibitset = COW_CREATE_OWNED(hibitset, b),
        .current_bit_index = hibitset_bit_range(&b).start,
    };
}

hibitset_iterator hibitset_iterator_create_owned_at_last(const hibitset b) {
    return (hibitset_iterator){
        .hibitset = COW_CREATE_OWNED(hibitset, b),
        .current_bit_index = hibitset_bit_range(&b).end - 1,
    };
}

inline bool hibitset_iterator_done(const hibitset_iterator *it) {
    return !hibitset_bit_in_range(COW_GET_REFERENCE(hibitset, it->hibitset), it->current_bit_index);
}

inline size_t hibitset_iterator_next(hibitset_iterator *it) {
    return ++it->current_bit_index;
}

inline size_t hibitset_iterator_previous(hibitset_iterator *it) {
    return --it->current_bit_index;
}

size_t hibitset_iterator_next_set(hibitset_iterator *it) {
    ssize_t unset_bit_skip_count = 1;
    do {
        it->current_bit_index += unset_bit_skip_count;
    } while (!hibitset_iterator_done(it)
        && !hibitset_is_set_skip_unset(COW_GET_REFERENCE(hibitset, it->hibitset), it->current_bit_index, &unset_bit_skip_count));
    return it->current_bit_index;
}

size_t hibitset_iterator_previous_set(hibitset_iterator *it) {
    ssize_t unset_bit_skip_count = 1;
    do {
        it->current_bit_index -= unset_bit_skip_count;
    } while (!hibitset_iterator_done(it)
        && !hibitset_is_set_skip_unset_reverse(COW_GET_REFERENCE(hibitset, it->hibitset), it->current_bit_index, &unset_bit_skip_count)
        && (ssize_t)it->current_bit_index >= unset_bit_skip_count);
    return it->current_bit_index;
}

inline size_t hibitset_iterator_current(const hibitset_iterator *it) {
    return it->current_bit_index;
}

inline bool hibitset_iterator_current_is_set(const hibitset_iterator *it) {
    return hibitset_is_set(COW_GET_REFERENCE(hibitset, it->hibitset), hibitset_iterator_current(it));
}


// TODO: test hibitset operations onto, result mutates parameter
hibitset hibitset_intersection(const hibitset *bitsets, size_t count, arena *a) {
    assert(count >= 2 && "attempted to compute intersection of less than 2 bitsets");
    exclusive_range intersection_range = hibitset_bit_range(&bitsets[0]);
    for (size_t i = 1; i < count; i++) {
        intersection_range = exclusive_range_from(
            range_intersection(intersection_range.range, hibitset_bit_range(&bitsets[i]).range)
        );
    }

    hibitset b = hibitset_create(a);
    size_t current_bit = intersection_range.start;
    bool any_done = false;

    while (!any_done) {
        size_t max_next_bit = current_bit + 1;
        bool all_set = true;

        for (size_t i = 0; i < count; i++) {
            hibitset_iterator j = hibitset_iterator_create_borrowed_at(&bitsets[i], current_bit);
            size_t next_bit = hibitset_iterator_next_set(&j);
            max_next_bit = max(max_next_bit, next_bit);

            all_set &= hibitset_is_set(&bitsets[i], current_bit);
            any_done |= !hibitset_bit_in_range(&bitsets[i], max_next_bit);
        }
        if (all_set) {
            hibitset_set(&b, a, current_bit);
        }
        
        current_bit = max_next_bit;
    }
    return b;
}

hibitset hibitset_union(const hibitset *bitsets, size_t count, arena *a) {
    assert(count >= 2 && "attempted to compute union of less than 2 bitsets");
    exclusive_range union_range = hibitset_bit_range(&bitsets[0]);
    for (size_t i = 1; i < count; i++) {
        union_range = exclusive_range_from(
            range_union(union_range.range, hibitset_bit_range(&bitsets[i]).range)
        );
    }

    hibitset b = hibitset_create(a);
    size_t current_bit = union_range.start;
    bool all_done = false;

    while (!all_done) {
        size_t min_next_bit = SIZE_MAX;
        bool any_set = false;
        bool all_done_at_current = true;

        for (size_t i = 0; i < count; i++) {
            hibitset_iterator j = hibitset_iterator_create_borrowed_at(&bitsets[i], current_bit);
            size_t next_bit = hibitset_iterator_next_set(&j);
            min_next_bit = min(min_next_bit, next_bit);

            any_set |= hibitset_is_set(&bitsets[i], current_bit);
            all_done_at_current &= !hibitset_bit_in_range(&bitsets[i], min_next_bit);
        }
        if (any_set) {
            hibitset_set(&b, a, current_bit);
        }

        current_bit = min_next_bit;
        all_done = all_done_at_current;
    }
    return b;
}

hibitset hibitset_difference(const hibitset *bitset, const hibitset *subtracted_bitsets, size_t count, arena *a) {
    assert(count >= 1 && "attempted to compute difference of less than 2 bitsets");
    hibitset b = hibitset_create(a);

    hibitset_iterator it = hibitset_iterator_create_borrowed_at_first(bitset);
    if (!hibitset_iterator_current_is_set(&it)) {
        hibitset_iterator_next_set(&it);
    }
    
    while (!hibitset_iterator_done(&it)) {
        bool any_set = false;
        for (size_t i = 0; i < count; i++) {
            any_set |= hibitset_is_set(&subtracted_bitsets[i], it.current_bit_index);
        }

        if (!any_set) {
            hibitset_set(&b, a, it.current_bit_index);
        }
        hibitset_iterator_next_set(&it);
    }
    return b;
}


hibitset *hibitset_intersect(hibitset *self, const hibitset *bitsets, size_t count, arena *a) {
    assert(count >= 1 && "attempted to intersect less than 2 bitsets");
    hibitset_iterator it = hibitset_iterator_create_borrowed_at_first(self);
    if (!hibitset_iterator_current_is_set(&it)) {
        hibitset_iterator_next_set(&it);
    }

    while (!hibitset_iterator_done(&it)) {
        bool any_unset = false;
        for (size_t i = 0; i < count && !any_unset; i++) {
            any_unset |= !hibitset_is_set(&bitsets[i], it.current_bit_index);
        }
        if (any_unset) {
            hibitset_unset(self, a, it.current_bit_index);
        }
        hibitset_iterator_next_set(&it);
    }
    return self;
}

hibitset *hibitset_join(hibitset *self, const hibitset *bitsets, size_t count, arena *a) {
    assert(count >= 1 && "attempted to join less than 2 bitsets");
    exclusive_range union_range = hibitset_bit_range(self);
    for (size_t i = 0; i < count; i++) {
        union_range = exclusive_range_from(
            range_union(union_range.range, hibitset_bit_range(&bitsets[i]).range)
        );
    }

    size_t current_bit = union_range.start;
    bool all_done = false;

    while (!all_done) {
        size_t min_next_bit = SIZE_MAX;
        bool any_set = false;
        bool all_done_at_current = true;

        for (size_t i = 0; i < count; i++) {
            hibitset_iterator j = hibitset_iterator_create_borrowed_at(&bitsets[i], current_bit);
            size_t next_bit = hibitset_iterator_next_set(&j);
            min_next_bit = min(min_next_bit, next_bit);

            any_set |= hibitset_is_set(&bitsets[i], current_bit);
            all_done_at_current &= !hibitset_bit_in_range(&bitsets[i], min_next_bit);
        }
        if (any_set) {
            hibitset_set(self, a, current_bit);
        }

        current_bit = min_next_bit;
        all_done = all_done_at_current;
    }
    return self;
}

hibitset *hibitset_subtract(hibitset *self, const hibitset *subtracted_bitsets, size_t count, arena *a) {
    assert(count >= 1 && "attempted to subtract less than 2 bitsets");
    hibitset_iterator it = hibitset_iterator_create_borrowed_at_first(self);
    if (!hibitset_iterator_current_is_set(&it)) {
        hibitset_iterator_next_set(&it);
    }
    
    while (!hibitset_iterator_done(&it)) {
        bool any_set = false;
        for (size_t i = 0; i < count && !any_set; i++) {
            any_set |= hibitset_is_set(&subtracted_bitsets[i], it.current_bit_index);
        }
        if (any_set) {
            hibitset_unset(self, a, it.current_bit_index);
        }

        hibitset_iterator_next_set(&it);
    }
    return self;
}


#endif