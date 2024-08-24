#ifndef BITSET_H
#define BITSET_H

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
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
_STATIC_ASSERT(BIT_WORD_BITS == BIT_WORD_BIT_COUNT);

#define BIT_LAYER_COUNT (BIT_WORD_BITS_LOG2 - BIT_PAGE_SIZE_LOG2)

typedef exclusive_range layer_range;

typedef struct bitset {
    list bit_words[BIT_LAYER_COUNT];
    layer_range layer_ranges[BIT_LAYER_COUNT];
} bitset;

inline size_t layer_complement(size_t layer) {
    return BIT_LAYER_COUNT - layer - 1;
}

bitset bitset_create(arena *a) {
    list bit_words[BIT_LAYER_COUNT];
    for (size_t layer = 0; layer < BIT_LAYER_COUNT; layer++) {
        bit_words[layer] = list_create(a, sizeof(bit_word) << (BIT_PAGE_SIZE_LOG2 * layer_complement(layer)));
    }
    bitset b = {
        .bit_words = bit_words,
        .layer_ranges = {0},
    };
    return b;
}

inline size_t layer_word_index(size_t bit_index, size_t layer) { 
    return bit_index >> (BIT_WORD_BITS_LOG2 + layer * BIT_PAGE_SIZE_LOG2);
}

inline size_t layer_bit_index(size_t bit_index, size_t layer) {
    return bit_index >> (layer * BIT_PAGE_SIZE_LOG2);
}

inline size_t layer_word_bit_index(size_t bit_index, size_t layer) {
    return layer_bit_index(bit_index, layer) & (BIT_WORD_BIT_COUNT - 1);
}

void bit_set_ensure_layer_range(bitset *b, arena *a, layer_range range, size_t layer) {
    if (range.start < b->layer_ranges[layer].start) {
        size_t missing_count = b->layer_ranges[layer].start - range.start;
        bit_word *buffer = calloc(missing_count, sizeof(bit_word));
        LIST_INSERT_RANGE(bit_word, &b->bit_words[layer], a, 0, buffer, missing_count);
        free(buffer);
        b->layer_ranges[layer].start = range.start;
    } else if (range.end + 1 >= b->layer_ranges[layer].end) {
        size_t missing_count = range.end + 1 - b->layer_ranges[layer].end;
        bit_word *buffer = calloc(missing_count, sizeof(bit_word));
        LIST_ADD_RANGE(bit_word, &b->bit_words[layer], a, buffer, missing_count);
        free(buffer);
        b->layer_ranges[layer].end = range.end + 1;
    }
}

void bitset_set(bitset *b, arena *a, size_t index) {
    for (ssize_t layer = BIT_LAYER_COUNT; layer >= 0; layer--) {
        size_t layer_index = layer_word_index(index, layer);
        bit_set_ensure_layer_range(b, a, layer_range_singleton(layer_index), layer);
        ssize_t layer_list_index = (ssize_t)layer_index - b->layer_ranges[layer].first_bit_word_index;
        assert(
            (layer_list_index >= 0)
            && (layer_list_index < layer_range_length(b->layer_ranges[layer]))
            && "Index out of bounds"
        );
        bit_word *w = LIST_GET(bit_word, &b->bit_words[layer], layer_list_index);
        *w |= (bit_word)1 << layer_word_bit_index(index, layer);
    }
}

void bitset_unset(bitset *b, arena *a, size_t index) {
    bool page_unset = true;
    for (size_t layer = 0; layer < BIT_LAYER_COUNT && page_unset; layer++) {
        size_t layer_index = layer_word_index(index, layer);
        bit_set_ensure_layer_range(b, a, exclusive_range_singleton(layer_index), layer);
        ssize_t layer_list_index = (ssize_t)layer_index - b->layer_ranges[layer].start;
        assert(
            (layer_list_index >= 0)
            && (layer_list_index < layer_range_length(b->layer_ranges[layer]))
            && "Index out of bounds"
        );
        bit_word *w = LIST_GET(bit_word, &b->bit_words[layer], layer_list_index);
        size_t word_bit_index = layer_word_bit_index(index, layer);
        *w &= ~(bit_word)1 << word_bit_index;

        size_t page_index = word_bit_index / BIT_PAGE_SIZE;
        bit_word page = (*w >> (page_index * BIT_PAGE_SIZE)) & ~((bit_word)1 << ((page_index + 1) * BIT_PAGE_SIZE));
        page_unset = page == 0;
    }
}

bool bitset_is_set_skip_check(const bitset *b, size_t index, ssize_t *bit_skip_count) {
    *bit_skip_count = 1;
    for (ssize_t layer = BIT_LAYER_COUNT; layer >= 0; layer--) {
        size_t layer_index = layer_word_index(index, layer);
        size_t word_bit_index = layer_word_bit_index(index, layer);
        if (!exclusive_range_contains(b->layer_ranges[layer], layer_index)) {
            ssize_t word_skip_count = b->layer_ranges[layer].start - layer_index;
            *bit_skip_count = word_skip_count >= 0 ?
                word_skip_count << (BIT_WORD_BITS_LOG2 + layer * BIT_PAGE_SIZE_LOG2)              
                    + (BIT_WORD_BIT_COUNT - word_bit_index - 1) << (BIT_PAGE_SIZE_LOG2 * layer)
                : -1;
            return false;
        }
        ssize_t layer_list_index = (ssize_t)layer_index - b->layer_ranges[layer].start;
        bit_word *w = LIST_GET(bit_word, &b->bit_words[layer], layer_list_index);
        if ((*w & ((bit_word)1 << word_bit_index)) == 0) {
            *bit_skip_count = (BIT_WORD_BIT_COUNT - word_bit_index - 1) << (BIT_PAGE_SIZE_LOG2 * layer);
            return false;
        }
    }
    return true;
}

bool bitset_is_set(const bitset *b, size_t index) {
    for (ssize_t layer = BIT_LAYER_COUNT; layer >= 0; layer--) {
        size_t layer_index = layer_word_index(index, layer);
        if (!exclusive_range_contains(b->layer_ranges[layer], layer_index)) {
            return false;
        }
        ssize_t layer_list_index = (ssize_t)layer_index - b->layer_ranges[layer].start;
        bit_word *w = LIST_GET(bit_word, &b->bit_words[layer], layer_list_index);
        size_t word_bit_index = layer_word_bit_index(index, layer);
        if ((*w & ((bit_word)1 << word_bit_index)) == 0) {
            return false;
        }
    }
    return true;
}


typedef struct bitset_iterator_descriptor {
    bitset *bitset;
} bitset_iterator_descriptor;

typedef struct bitset_iterator {
    const bitset *const bitset;
    size_t current_bit;
} bitset_iterator;

bitset_iterator bitset_iterator_create(const bitset *b) {
    return (bitset_iterator) {
        .bitset = b,
        .current_bit = 0
    };
}

bitset_iterator bitset_iterator_from(bitset_iterator_descriptor descriptor) {
    return bitset_iterator_create(descriptor.bitset);
}

bool bitset_iterator_current(const bitset_iterator *iterator) {
    return bitset_is_set(iterator->bitset, iterator->current_bit);
}

inline size_t bitset_iterator_next(bitset_iterator *iterator) {
    return ++iterator->current_bit;
}

size_t bitset_iterator_next_set(bitset_iterator *iterator) {
    ssize_t bit_skip = 1;
    while (!bitset_is_set_skip_check(iterator->bitset, iterator->current_bit, &bit_skip) && bit_skip > 0) {
        iterator->current_bit += bit_skip;
    }
    return iterator->current_bit;
}

size_t bitset_iterator_next_unset(bitset_iterator *iterator) {
    ssize_t bit_skip = 1;
    while (bitset_is_set_skip_check(iterator->bitset, iterator->current_bit, &bit_skip) && bit_skip > 0) {
        iterator->current_bit += bit_skip;
    }
    return iterator->current_bit;
}

inline bool bitset_iterator_done(const bitset_iterator *iterator) {
    return iterator->current_bit >= ((iterator->bitset->layer_ranges[0].end - 1) << BIT_WORD_BITS_LOG2);
}


bitset bitset_intersection(VIEW_STRUCT(bitset) bitsets, arena *a) {
    layer_range layer0_intersection = (layer_range) {
        .start = bitsets.elements[0].layer_ranges[0].start,
        .start = bitsets.elements[0].layer_ranges[0].start
    };
    for (size_t i = 1; i < bitsets.count; i++) {
        layer0_intersection = exclusive_range_from(
            range_intersection(layer0_intersection.range, bitsets.elements[i].layer_ranges[0].range)
        );
    }
    if (exclusive_range_is_empty(layer0_intersection)) {
        return bitset_create(a);
    }

    bitset b = bitset_create(a);
    bitset_iterator it = bitset_iterator_create(&bitsets.elements[0]);
    it.current_bit = layer0_intersection.start << BIT_WORD_BITS_LOG2;

    while (!bitset_iterator_done(&it)) {
        bool all_set = true;
        for (size_t i = 0; i < bitsets.count && all_set; i++) {
            all_set &= bitset_is_set(&bitsets.elements[i], it.current_bit);
        }
        if (all_set) {
            bitset_set(&b, a, it.current_bit);
        }
        bitset_iterator_next_set(&it);
    }

    return b;
}

bitset bitset_union(VIEW_STRUCT(bitset) bitsets, arena *a) {
    layer_range layer0_union = (layer_range) {
        .start = bitsets.elements[0].layer_ranges[0].start,
        .start = bitsets.elements[0].layer_ranges[0].start
    };

    for (size_t i = 1; i < bitsets.count; i++) {
        layer0_union = exclusive_range_from(
            range_union(layer0_union.range, bitsets.elements[i].layer_ranges[0].range)
        );
    }
    if (exclusive_range_is_empty(layer0_union)) {
        return bitset_create(a);
    }

    bitset b = bitset_create(a);
    bitset_iterator it = bitset_iterator_create(&bitsets.elements[0]);
    it.current_bit = layer0_union.start << BIT_WORD_BITS_LOG2;

    while (!bitset_iterator_done(&it)) {
        bool any_set = false;
        for (size_t i = 0; i < bitsets.count && !any_set; i++) {
            any_set |= bitset_is_set(&bitsets.elements[i], it.current_bit);
        }
        if (any_set) {
            bitset_set(&b, a, it.current_bit);
        }
        bitset_iterator_next_set(&it);
    }

    return b;
}

#endif