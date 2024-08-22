#ifndef BITSET_H
#define BITSET_H

#include <stdint.h>
#include <assert.h>
#include "list.h"

#define BIT_PAGE_SIZE_LOG2 2
#define BIT_PAGE_SIZE (1 << BIT_PAGE_SIZE_LOG2)

#define BIT_WORD_BITS_LOG2 6
#define BIT_WORD_BITS (1 << BIT_WORD_BITS_LOG2)
#define BIT_WORD_TYPE uint64_t

typedef BIT_WORD_TYPE bit_word;
#define BIT_WORD_BIT_COUNT (8 * sizeof(bit_word))
_STATIC_ASSERT(BIT_WORD_BITS == BIT_WORD_BIT_COUNT);

#define BIT_LAYER_COUNT (BIT_WORD_BITS_LOG2 - BIT_PAGE_SIZE_LOG2)

typedef struct bitset {
    list bit_words;
    size_t layer_length[BIT_LAYER_COUNT];
} bitset;

bitset bitset_create(arena *a) {
    bitset b = {
        .bit_words = list_create(a, sizeof(bit_word)),
        .layer_length = {0}
    };
    return b;
}

inline size_t layer_word_index(size_t bit_index, size_t layer) { 
    return (bit_index >> (BIT_PAGE_SIZE_LOG2 * (BIT_WORD_BITS_LOG2 - layer)));
}

inline size_t layer_bit_index(size_t bit_index, size_t layer) {
    return (bit_index >> (BIT_PAGE_SIZE_LOG2 * (BIT_LAYER_COUNT - (layer + 1)))) & (BIT_WORD_BITS - 1);
}

void bitset_set(bitset *b, arena *a, size_t index) {
    size_t layer0_index = (index >> BIT_PAGE_SIZE) >> sizeof(bit_word);
    while (layer0_index >= b->layer_length[0]) {
        list_insert(&b->bit_words, a, b->layer_length[0]++, &((bit_word){0}), sizeof(bit_word));
    }


}

#endif