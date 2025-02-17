#include <assert.h>
#include <stdlib.h>

#include "cecs_bitset.h"

cecs_bitset cecs_bitset_create(cecs_arena* a, size_t capacity) {
    cecs_bitset b = (cecs_bitset){
        .bit_words = CECS_DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(cecs_bit_word, a, capacity),
        .word_range = { 0, 0 },
    };
    //bit_word *buffer = calloc(capacity, sizeof(bit_word));
    //CECS_DYNAMIC_ARRAY_ADD_RANGE(bit_word, &b.bit_words, a, buffer, capacity);
    //free(buffer);
    return b;
}

void cecs_bitset_unset_all(cecs_bitset* b) {
    cecs_dynamic_array_clear(&b->bit_words);
    b->word_range = (cecs_exclusive_range){ 0, 0 };
}

cecs_word_range cecs_bitset_expand(cecs_bitset* b, cecs_arena* a, size_t word_index) {
    if (cecs_exclusive_range_is_empty(b->word_range)) {
        assert(CECS_DYNAMIC_ARRAY_COUNT(cecs_bit_word, &b->bit_words) == 0);
        CECS_DYNAMIC_ARRAY_ADD(cecs_bit_word, &b->bit_words, a, &((cecs_bit_word) { 0 }));
        return (b->word_range = cecs_exclusive_range_singleton(word_index));
    }
    cecs_word_range expanded_range = cecs_exclusive_range_from(
        cecs_range_union(b->word_range.range, cecs_exclusive_range_singleton(word_index).range)
    );
    cecs_range_splitting expansion_ranges = cecs_exclusive_range_difference(
        expanded_range,
        b->word_range
    );

    cecs_word_range range0 = cecs_exclusive_range_from(expansion_ranges.ranges[0]);
    cecs_word_range range1 = cecs_exclusive_range_from(expansion_ranges.ranges[1]);
    if (!cecs_exclusive_range_is_empty(range0)) {
        size_t missing_count = cecs_exclusive_range_length(range0);
        cecs_bit_word* buffer = calloc(missing_count, sizeof(cecs_bit_word));
        CECS_DYNAMIC_ARRAY_INSERT_RANGE(cecs_bit_word, &b->bit_words, a, 0, buffer, missing_count);
        free(buffer);
    }
    else if (!cecs_exclusive_range_is_empty(range1)) {
        size_t missing_count = cecs_exclusive_range_length(range1);
        cecs_bit_word* buffer = calloc(missing_count, sizeof(cecs_bit_word));
        CECS_DYNAMIC_ARRAY_ADD_RANGE(cecs_bit_word, &b->bit_words, a, buffer, missing_count);
        free(buffer);
    }

    return (b->word_range = expanded_range);
}

cecs_bit_word cecs_bitset_set(cecs_bitset* b, cecs_arena* a, size_t bit_index) {
    size_t word_index = cecs_layer_word_index(bit_index, 0);
    if (!cecs_exclusive_range_contains(b->word_range, word_index)) {
        cecs_bitset_expand(b, a, word_index);
    }
    cecs_ssize_t cecs_dynamic_array_index = (cecs_ssize_t)word_index - b->word_range.start;
    assert(
        cecs_dynamic_array_index >= 0
        && cecs_dynamic_array_index < cecs_exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );
    cecs_bit_word* w = CECS_DYNAMIC_ARRAY_GET(cecs_bit_word, &b->bit_words, cecs_dynamic_array_index);
    return (*w |= (cecs_bit_word)1 << cecs_layer_word_bit_index(bit_index, 0));
}

size_t cecs_bitset_set_range(
    cecs_bitset *b,
    cecs_arena *a,
    size_t bit_index,
    size_t count,
    const cecs_bit_word **out_set_words
) {
    if (count == 0) {
        *out_set_words = NULL;
        return 0;
    }
    
    size_t first_word_index = cecs_layer_word_index(bit_index, 0);
    if (!cecs_exclusive_range_contains(b->word_range, first_word_index)) {
        cecs_bitset_expand(b, a, first_word_index);
    }
    size_t last_bit_index = bit_index + count - 1;
    size_t last_word_index = cecs_layer_word_index(last_bit_index, 0);
    if (!cecs_exclusive_range_contains(b->word_range, last_word_index)) {
        cecs_bitset_expand(b, a, last_word_index);
    }

    cecs_ssize_t displaced_first_word_index = (cecs_ssize_t)first_word_index - b->word_range.start;
    assert(
        displaced_first_word_index >= 0
        && displaced_first_word_index < cecs_exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );
    cecs_ssize_t displaced_last_word_index = (cecs_ssize_t)last_word_index - b->word_range.start;
    assert(
        displaced_last_word_index >= 0
        && displaced_last_word_index < cecs_exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );

    size_t last_right_bit_shift = CECS_BIT_WORD_BIT_COUNT - cecs_layer_word_bit_index(last_bit_index, 0) - 1;
    size_t first_left_bit_shift = cecs_layer_word_bit_index(bit_index, 0);
    cecs_bit_word *first_word = CECS_DYNAMIC_ARRAY_GET(cecs_bit_word, &b->bit_words, displaced_first_word_index);
    cecs_bit_word *last_word = CECS_DYNAMIC_ARRAY_GET(cecs_bit_word, &b->bit_words, displaced_last_word_index);
    size_t set_word_count = (1 + last_word_index) - first_word_index;
    switch (set_word_count) {
    case 0: {
        assert(false && "unreachable: bit count is zero");
        exit(EXIT_FAILURE);
        break;
    }
    case 1: {
        cecs_bit_word mask = (CECS_BIT_WORD_MAX >> last_right_bit_shift) & (CECS_BIT_WORD_MAX << first_left_bit_shift);
        assert(CECS_BIT_WORD_BIT_COUNT - first_left_bit_shift - last_right_bit_shift == count && "fatal error: bit count mismatch");
        *first_word |= mask;
        break;
    }
    case 2: {
        cecs_bit_word first_mask = CECS_BIT_WORD_MAX << first_left_bit_shift;
        cecs_bit_word last_mask = CECS_BIT_WORD_MAX >> last_right_bit_shift;
        assert(
            2 * CECS_BIT_WORD_BIT_COUNT - first_left_bit_shift - last_right_bit_shift == count
            && "fatal error: bit count mismatch"
        );
        *first_word |= first_mask;
        *last_word |= last_mask;
        break;
    }
    default: {
        cecs_bit_word first_mask = CECS_BIT_WORD_MAX << first_left_bit_shift;
        cecs_bit_word last_mask = CECS_BIT_WORD_MAX >> last_right_bit_shift;
        assert(
            (2 * CECS_BIT_WORD_BIT_COUNT - first_left_bit_shift - last_right_bit_shift
                + CECS_BIT_WORD_BIT_COUNT * (set_word_count - 2)) == count
            && "fatal error: bit count mismatch"
        );
        *first_word |= first_mask;
        CECS_DYNAMIC_ARRAY_SET_COPY_RANGE(
            cecs_bit_word,
            &b->bit_words,
            displaced_first_word_index + 1,
            &((cecs_bit_word) { CECS_BIT_WORD_MAX }),
            set_word_count - 1
        );
        *last_word |= last_mask;
        break;
    }
    }
    *out_set_words = first_word;
    return set_word_count;
}

cecs_bit_word cecs_bitset_unset(cecs_bitset* b, cecs_arena* a, size_t bit_index) {
    size_t word_index = cecs_layer_word_index(bit_index, 0);
    if (!cecs_exclusive_range_contains(b->word_range, word_index)) {
        cecs_bitset_expand(b, a, word_index);
    }
    cecs_ssize_t cecs_dynamic_array_index = (cecs_ssize_t)word_index - b->word_range.start;
    assert(
        cecs_dynamic_array_index >= 0
        && cecs_dynamic_array_index < cecs_exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );
    cecs_bit_word* w = CECS_DYNAMIC_ARRAY_GET(cecs_bit_word, &b->bit_words, cecs_dynamic_array_index);
    return (*w &= ~((cecs_bit_word)1 << cecs_layer_word_bit_index(bit_index, 0)));
}

size_t cecs_bitset_unset_range(
    cecs_bitset *b,
    cecs_arena *a,
    size_t bit_index,
    size_t count,
    const cecs_bit_word **out_unset_words
) {
    if (count == 0) {
        *out_unset_words = NULL;
        return 0;
    }
    
    size_t first_word_index = cecs_layer_word_index(bit_index, 0);
    if (!cecs_exclusive_range_contains(b->word_range, first_word_index)) {
        cecs_bitset_expand(b, a, first_word_index);
    }
    size_t last_bit_index = bit_index + count - 1;
    size_t last_word_index = cecs_layer_word_index(last_bit_index, 0);
    if (!cecs_exclusive_range_contains(b->word_range, last_word_index)) {
        cecs_bitset_expand(b, a, last_word_index);
    }

    cecs_ssize_t displaced_first_word_index = (cecs_ssize_t)first_word_index - b->word_range.start;
    assert(
        displaced_first_word_index >= 0
        && displaced_first_word_index < cecs_exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );
    cecs_ssize_t displaced_last_word_index = (cecs_ssize_t)last_word_index - b->word_range.start;
    assert(
        displaced_last_word_index >= 0
        && displaced_last_word_index < cecs_exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );

    size_t last_right_bit_shift = CECS_BIT_WORD_BIT_COUNT - cecs_layer_word_bit_index(last_bit_index, 0) - 1;
    size_t first_left_bit_shift = cecs_layer_word_bit_index(bit_index, 0);
    cecs_bit_word *first_word = CECS_DYNAMIC_ARRAY_GET(cecs_bit_word, &b->bit_words, displaced_first_word_index);
    cecs_bit_word *last_word = CECS_DYNAMIC_ARRAY_GET(cecs_bit_word, &b->bit_words, displaced_last_word_index);
    size_t unset_word_count = (1 + last_word_index) - first_word_index;
    switch (unset_word_count) {
    case 0: {
        assert(false && "unreachable: bit count is zero");
        exit(EXIT_FAILURE);
        break;
    }
    case 1: {
        cecs_bit_word mask = (CECS_BIT_WORD_MAX >> last_right_bit_shift) & (CECS_BIT_WORD_MAX << first_left_bit_shift);
        assert(CECS_BIT_WORD_BIT_COUNT - first_left_bit_shift - last_right_bit_shift == count && "fatal error: bit count mismatch");
        *first_word &= ~mask;
        break;
    }
    case 2: {
        cecs_bit_word first_mask = CECS_BIT_WORD_MAX << first_left_bit_shift;
        cecs_bit_word last_mask = CECS_BIT_WORD_MAX >> last_right_bit_shift;
        assert(
            2 * CECS_BIT_WORD_BIT_COUNT - first_left_bit_shift - last_right_bit_shift == count
            && "fatal error: bit count mismatch"
        );
        *first_word &= ~first_mask;
        *last_word &= ~last_mask;
        break;
    }
    default: {
        cecs_bit_word first_mask = CECS_BIT_WORD_MAX << first_left_bit_shift;
        cecs_bit_word last_mask = CECS_BIT_WORD_MAX >> last_right_bit_shift;
        assert(
            (2 * CECS_BIT_WORD_BIT_COUNT - first_left_bit_shift - last_right_bit_shift
                + CECS_BIT_WORD_BIT_COUNT * (unset_word_count - 2)) == count
            && "fatal error: bit count mismatch"
        );
        *first_word &= ~first_mask;
        CECS_DYNAMIC_ARRAY_SET_COPY_RANGE(
            cecs_bit_word,
            &b->bit_words,
            displaced_first_word_index + 1,
            &((cecs_bit_word) { 0 }),
            unset_word_count - 1
        );
        *last_word &= ~last_mask;
        break;
    }
    }
    *out_unset_words = first_word;
    return unset_word_count;
}

cecs_bit_word cecs_bitset_get_word(const cecs_bitset* b, size_t bit_index) {
    size_t word_index = cecs_layer_word_index(bit_index, 0);
    if (!cecs_exclusive_range_contains(b->word_range, word_index)) {
        return (cecs_bit_word)0;
    }
    cecs_ssize_t cecs_dynamic_array_index = (cecs_ssize_t)word_index - b->word_range.start;
    cecs_bit_word* w = CECS_DYNAMIC_ARRAY_GET(cecs_bit_word, &b->bit_words, cecs_dynamic_array_index);
    return *w;
}

bool cecs_bitset_is_set(const cecs_bitset* b, size_t bit_index) {
    return (cecs_bitset_get_word(b, bit_index)
        & ((cecs_bit_word)1 << cecs_layer_word_bit_index(bit_index, 0))) != 0;
}

bool cecs_bitset_bit_in_range(const cecs_bitset* b, size_t bit_index) {
    return cecs_exclusive_range_contains(b->word_range, cecs_layer_word_index(bit_index, 0));
}

cecs_bitset_iterator cecs_bitset_iterator_create(const cecs_bitset* b) {
    return (cecs_bitset_iterator) {
        .bitset = b,
            .current_bit_index = cecs_bit0_from_layer_word_index(b->word_range.start, 0),
    };
}

size_t cecs_bitset_iterator_next_set(cecs_bitset_iterator* it) {
    do {
        cecs_bitset_iterator_next(it);
    } while (!cecs_bitset_iterator_done(it) && !cecs_bitset_is_set(it->bitset, it->current_bit_index));
    return it->current_bit_index;
}

size_t cecs_bitset_iterator_next_unset(cecs_bitset_iterator* it) {
    do {
        cecs_bitset_iterator_next(it);
    } while (!cecs_bitset_iterator_done(it) && cecs_bitset_is_set(it->bitset, it->current_bit_index));
    return it->current_bit_index;
}

cecs_hibitset cecs_hibitset_create(cecs_arena* a) {
    cecs_hibitset b;
    for (size_t layer = 0; layer < CECS_BIT_LAYER_COUNT; layer++) {
        b.bitsets[layer] = cecs_bitset_create(a, (size_t)1 << (CECS_BIT_PAGE_SIZE_LOG2 * cecs_layer_complement(layer)));
    }
    return b;
}

void cecs_hibitset_unset_all(cecs_hibitset* b) {
    for (size_t layer = 0; layer < CECS_BIT_LAYER_COUNT; layer++) {
        cecs_bitset_unset_all(&b->bitsets[layer]);
    }
}

void cecs_hibitset_set(cecs_hibitset* b, cecs_arena* a, size_t bit_index) {
    for (cecs_ssize_t layer = CECS_BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = cecs_layer_bit_index(bit_index, layer);
        cecs_bitset_set(&b->bitsets[layer], a, layer_bit);
    }
}

void cecs_hibitset_set_range(cecs_hibitset *b, cecs_arena *a, size_t bit_index, size_t count) {
    if (count == 0) {
        return;
    }

    const cecs_bit_word *set_words;
    for (cecs_ssize_t layer = CECS_BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t first_layer_bit = cecs_layer_bit_index(bit_index, layer);
        size_t last_layer_bit = cecs_layer_bit_index(bit_index + count - 1, layer);
        size_t bit_count = (1 + last_layer_bit) - first_layer_bit;

        cecs_bitset_set_range(&b->bitsets[layer], a, first_layer_bit, bit_count, &set_words);
    }
    (void)set_words;
}

void cecs_hibitset_unset(cecs_hibitset* b, cecs_arena* a, size_t bit_index) {
    for (size_t layer = 0; layer < CECS_BIT_LAYER_COUNT; layer++) {
        size_t layer_bit = cecs_layer_bit_index(bit_index, layer);
        cecs_bit_word unset_word = cecs_bitset_unset(&b->bitsets[layer], a, layer_bit);

        size_t layer_word_bit_page = cecs_layer_word_bit_index(bit_index, layer) >> CECS_BIT_PAGE_SIZE_LOG2;
        cecs_bit_word unset_word_page = unset_word & cecs_page_mask(layer_word_bit_page);

        if (unset_word_page != 0) {
            break;
        }
    }
}

#define CECS_UNSET_BIT_RANGE_STRUCT \
    struct unset_bits_range { \
        size_t layer; \
        size_t bit_index; \
        size_t count; \
    }

static size_t cecs_hibitset_unset_range_push_layer(
    cecs_dynamic_array *unset_ranges,
    const cecs_bit_word *unset_words,
    size_t unset_word_count,
    cecs_arena *range_arena,
    size_t layer,
    size_t bit_index,
    size_t bit_count
) {
    typedef CECS_UNSET_BIT_RANGE_STRUCT unset_bits_range;
    assert(
        unset_ranges->count % sizeof(unset_bits_range) == 0
        && "fatal error: unset_ranges is not a dynamic array of unset_bits_range"
    );

    for (size_t i = 0; i < unset_word_count; ++i) {
        cecs_bit_word unset_word = unset_words[i];
        size_t layer_word_bit_page =
            cecs_layer_word_bit_index(bit_index + (bit_count * i / unset_word_count), layer)
            >> CECS_BIT_PAGE_SIZE_LOG2;
        cecs_bit_word unset_word_page = unset_word & cecs_page_mask(layer_word_bit_page);

        size_t next_layer = layer + 1;
        size_t next_bit_count = bit_count >> CECS_BIT_PAGE_SIZE_LOG2;
        if (unset_word_page == 0
            && next_layer < CECS_BIT_LAYER_COUNT
            && next_bit_count > 0) {
            CECS_DYNAMIC_ARRAY_ADD(
                unset_bits_range,
                unset_ranges,
                range_arena,
                &((unset_bits_range){
                    .layer = next_layer,
                    .bit_index = layer_word_bit_page,
                    .count = next_bit_count,
                })
            );
        }
    }

    return CECS_DYNAMIC_ARRAY_COUNT(unset_bits_range, unset_ranges);
}

void cecs_hibitset_unset_range(cecs_hibitset *b, cecs_arena *a, size_t bit_index, size_t count) {
    if (count == 0) {
        return;
    }

    typedef CECS_UNSET_BIT_RANGE_STRUCT unset_bits_range;
    cecs_arena range_arena = cecs_arena_create_with_capacity(
        count * sizeof(unset_bits_range) / 2
    );

    cecs_dynamic_array unset_ranges = cecs_dynamic_array_create();
    CECS_DYNAMIC_ARRAY_ADD(
        unset_bits_range,
        &unset_ranges,
        &range_arena,
        &((unset_bits_range){ 0, bit_index, count })
    );

    size_t unset_ranges_count = 1;
    while (unset_ranges_count > 0) {
        unset_bits_range current_range = *(unset_bits_range *)cecs_dynamic_array_last(&unset_ranges, sizeof(unset_bits_range));
        CECS_DYNAMIC_ARRAY_REMOVE(unset_bits_range, &unset_ranges, &range_arena, --unset_ranges_count);

        size_t first_layer_bit = cecs_layer_bit_index(current_range.bit_index, current_range.layer);
        size_t last_layer_bit = cecs_layer_bit_index(current_range.bit_index + current_range.count - 1, current_range.layer);
        size_t bit_count = (1 + last_layer_bit) - first_layer_bit;

        const cecs_bit_word *unset_words;
        size_t unset_word_count
            = cecs_bitset_unset_range(&b->bitsets[current_range.layer], a, first_layer_bit, bit_count, &unset_words);
        unset_ranges_count = cecs_hibitset_unset_range_push_layer(
            &unset_ranges,
            unset_words,
            unset_word_count,
            &range_arena,
            current_range.layer,
            first_layer_bit,
            bit_count
        );
    }
    cecs_arena_free(&range_arena);
}
#undef CECS_UNSET_BIT_RANGE_STRUCT

bool cecs_hibitset_is_set(const cecs_hibitset* b, size_t bit_index) {
    return cecs_bitset_is_set(&b->bitsets[0], bit_index);
}

bool cecs_hibitset_is_set_skip_unset(const cecs_hibitset* b, size_t bit_index, cecs_ssize_t* out_unset_bit_skip_count) {
    *out_unset_bit_skip_count = 1;
    for (cecs_ssize_t layer = CECS_BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = cecs_layer_bit_index(bit_index, layer);
        cecs_bit_word word = cecs_bitset_get_word(&b->bitsets[layer], layer_bit);
        size_t layer_word_bit_shift = layer_bit & (CECS_BIT_WORD_BIT_COUNT - 1);
        cecs_bit_word word_shifted_to_bit = word >> layer_word_bit_shift;

        if ((word_shifted_to_bit & (cecs_bit_word)1) == (cecs_bit_word)0) {
            unsigned long unset_low = 1;
            unsigned long unset_high = 0;
            unsigned long word_low = (unsigned long)word_shifted_to_bit;
            unsigned long word_high = (unsigned long)(word_shifted_to_bit >> (CECS_BIT_WORD_BIT_COUNT >> 1));

            size_t unset_continuous_count = 0;
            if (!_BitScanForward(&unset_low, word_low)) {
                if (!_BitScanForward(&unset_high, word_high)) {
                    unset_continuous_count = CECS_BIT_WORD_BIT_COUNT - layer_word_bit_shift;
                }
                else {
                    unset_continuous_count = unset_high + (CECS_BIT_WORD_BIT_COUNT >> 1);
                }
            }
            else {
                unset_continuous_count = unset_low;
            }
            *out_unset_bit_skip_count =
                cecs_bit0_from_layer_bit_index(layer_bit + unset_continuous_count, layer) - bit_index;
            assert(*out_unset_bit_skip_count > 0);
            return false;
        }
    }
    return true;
}

bool cecs_hibitset_is_set_skip_unset_reverse(const cecs_hibitset* b, size_t bit_index, cecs_ssize_t* out_unset_bit_skip_count) {
    *out_unset_bit_skip_count = 1;
    for (cecs_ssize_t layer = CECS_BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = cecs_layer_bit_index(bit_index, layer);
        cecs_bit_word word = cecs_bitset_get_word(&b->bitsets[layer], layer_bit);
        size_t layer_word_bit_shift = layer_bit & (CECS_BIT_WORD_BIT_COUNT - 1);
        cecs_bit_word word_shifted_to_bit = word >> layer_word_bit_shift;

        if ((word_shifted_to_bit & (cecs_bit_word)1) == (cecs_bit_word)0) {
            unsigned long unset_low = 1;
            unsigned long unset_high = 0;
            unsigned long word_low = (unsigned long)word_shifted_to_bit;
            unsigned long word_high = (unsigned long)(word_shifted_to_bit >> (CECS_BIT_WORD_BIT_COUNT >> 1));

            size_t unset_continuous_count = 0;
            if (!_BitScanReverse(&unset_low, word_low)) {
                if (!_BitScanReverse(&unset_high, word_high)) {
                    unset_continuous_count = CECS_BIT_WORD_BIT_COUNT - layer_word_bit_shift;
                }
                else {
                    unset_continuous_count = unset_high + (CECS_BIT_WORD_BIT_COUNT >> 1);
                }
            }
            else {
                unset_continuous_count = unset_low;
            }
            *out_unset_bit_skip_count =
                bit_index - cecs_bit0_from_layer_bit_index(layer_bit - unset_continuous_count, layer);
            assert(*out_unset_bit_skip_count > 0);
            return false;
        }
    }
    return true;
}

cecs_bit_word cecs_hibitset_get_word(const cecs_hibitset* b, size_t bit_index) {
    return cecs_bitset_get_word(&b->bitsets[0], bit_index);
}

cecs_exclusive_range cecs_hibitset_bit_range(const cecs_hibitset* b) {
    return (cecs_exclusive_range) {
        .start = cecs_bit0_from_layer_word_index(b->bitsets[0].word_range.start, 0),
            .end = cecs_bit0_from_layer_word_index(b->bitsets[0].word_range.end, 0),
    };
}

bool cecs_hibitset_bit_in_range(const cecs_hibitset* b, size_t bit_index) {
    return cecs_bitset_bit_in_range(&b->bitsets[0], bit_index);
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at(const cecs_hibitset* b, size_t bit_index) {
    return (cecs_hibitset_iterator) {
        .hibitset = CECS_COW_CREATE_BORROWED(cecs_hibitset, b),
            .current_bit_index = bit_index,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at_first(const cecs_hibitset* b) {
    return (cecs_hibitset_iterator) {
        .hibitset = CECS_COW_CREATE_BORROWED(cecs_hibitset, b),
            .current_bit_index = cecs_hibitset_bit_range(b).start,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at_last(const cecs_hibitset* b) {
    return (cecs_hibitset_iterator) {
        .hibitset = CECS_COW_CREATE_BORROWED(cecs_hibitset, b),
            .current_bit_index = cecs_hibitset_bit_range(b).end - 1,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at(const cecs_hibitset b, size_t bit_index) {
    return (cecs_hibitset_iterator) {
        .hibitset = CECS_COW_CREATE_OWNED(cecs_hibitset, b),
            .current_bit_index = bit_index,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at_first(const cecs_hibitset b) {
    return (cecs_hibitset_iterator) {
        .hibitset = CECS_COW_CREATE_OWNED(cecs_hibitset, b),
            .current_bit_index = cecs_hibitset_bit_range(&b).start,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at_last(const cecs_hibitset b) {
    return (cecs_hibitset_iterator) {
        .hibitset = CECS_COW_CREATE_OWNED(cecs_hibitset, b),
            .current_bit_index = cecs_hibitset_bit_range(&b).end - 1,
    };
}

size_t cecs_hibitset_iterator_next_set(cecs_hibitset_iterator* it) {
    cecs_ssize_t unset_bit_skip_count = 1;
    do {
        it->current_bit_index += unset_bit_skip_count;
    } while (!cecs_hibitset_iterator_done(it)
        && !cecs_hibitset_is_set_skip_unset(CECS_COW_GET_REFERENCE(cecs_hibitset, it->hibitset), it->current_bit_index, &unset_bit_skip_count));
    return it->current_bit_index;
}

size_t cecs_hibitset_iterator_previous_set(cecs_hibitset_iterator* it) {
    cecs_ssize_t unset_bit_skip_count = 1;
    do {
        it->current_bit_index -= unset_bit_skip_count;
    } while (!cecs_hibitset_iterator_done(it)
        && !cecs_hibitset_is_set_skip_unset_reverse(CECS_COW_GET_REFERENCE(cecs_hibitset, it->hibitset), it->current_bit_index, &unset_bit_skip_count)
        && (cecs_ssize_t)it->current_bit_index >= unset_bit_skip_count);
    return it->current_bit_index;
}

// TODO: test hibitset operations onto, result mutates parameter
cecs_hibitset cecs_hibitset_intersection(const cecs_hibitset* bitsets, size_t count, cecs_arena* a) {
    assert(count >= 2 && "attempted to compute intersection of less than 2 bitsets");
    cecs_exclusive_range intersection_range = cecs_hibitset_bit_range(&bitsets[0]);
    for (size_t i = 1; i < count; i++) {
        intersection_range = cecs_exclusive_range_from(
            cecs_range_intersection(intersection_range.range, cecs_hibitset_bit_range(&bitsets[i]).range)
        );
    }

    cecs_hibitset b = cecs_hibitset_create(a);
    size_t current_bit = intersection_range.start;
    bool any_done = false;

    while (!any_done) {
        size_t max_next_bit = current_bit + 1;
        bool all_set = true;

        for (size_t i = 0; i < count; i++) {
            cecs_hibitset_iterator j = cecs_hibitset_iterator_create_borrowed_at(&bitsets[i], current_bit);
            size_t next_bit = cecs_hibitset_iterator_next_set(&j);
            max_next_bit = max(max_next_bit, next_bit);

            all_set &= cecs_hibitset_is_set(&bitsets[i], current_bit);
            any_done |= !cecs_hibitset_bit_in_range(&bitsets[i], max_next_bit);
        }
        if (all_set) {
            cecs_hibitset_set(&b, a, current_bit);
        }

        current_bit = max_next_bit;
    }
    return b;
}

cecs_hibitset cecs_hibitset_union(const cecs_hibitset* bitsets, size_t count, cecs_arena* a) {
    assert(count >= 2 && "attempted to compute union of less than 2 bitsets");
    cecs_exclusive_range union_range = cecs_hibitset_bit_range(&bitsets[0]);
    for (size_t i = 1; i < count; i++) {
        union_range = cecs_exclusive_range_from(
            cecs_range_union(union_range.range, cecs_hibitset_bit_range(&bitsets[i]).range)
        );
    }

    cecs_hibitset b = cecs_hibitset_create(a);
    size_t current_bit = union_range.start;
    bool all_done = false;

    while (!all_done) {
        size_t min_next_bit = SIZE_MAX;
        bool any_set = false;
        bool all_done_at_current = true;

        for (size_t i = 0; i < count; i++) {
            cecs_hibitset_iterator j = cecs_hibitset_iterator_create_borrowed_at(&bitsets[i], current_bit);
            size_t next_bit = cecs_hibitset_iterator_next_set(&j);
            min_next_bit = min(min_next_bit, next_bit);

            any_set |= cecs_hibitset_is_set(&bitsets[i], current_bit);
            all_done_at_current &= !cecs_hibitset_bit_in_range(&bitsets[i], min_next_bit);
        }
        if (any_set) {
            cecs_hibitset_set(&b, a, current_bit);
        }

        current_bit = min_next_bit;
        all_done = all_done_at_current;
    }
    return b;
}

cecs_hibitset cecs_hibitset_difference(const cecs_hibitset* bitset, const cecs_hibitset* subtracted_bitsets, size_t count, cecs_arena* a) {
    assert(count >= 1 && "attempted to compute difference of less than 2 bitsets");
    cecs_hibitset b = cecs_hibitset_create(a);

    cecs_hibitset_iterator it = cecs_hibitset_iterator_create_borrowed_at_first(bitset);
    if (!cecs_hibitset_iterator_current_is_set(&it)) {
        cecs_hibitset_iterator_next_set(&it);
    }

    while (!cecs_hibitset_iterator_done(&it)) {
        bool any_set = false;
        for (size_t i = 0; i < count; i++) {
            any_set |= cecs_hibitset_is_set(&subtracted_bitsets[i], it.current_bit_index);
        }

        if (!any_set) {
            cecs_hibitset_set(&b, a, it.current_bit_index);
        }
        cecs_hibitset_iterator_next_set(&it);
    }
    return b;
}

cecs_hibitset* cecs_hibitset_intersect(cecs_hibitset* self, const cecs_hibitset* bitsets, size_t count, cecs_arena* a) {
    assert(count >= 1 && "attempted to intersect less than 2 bitsets");
    cecs_hibitset_iterator it = cecs_hibitset_iterator_create_borrowed_at_first(self);
    if (!cecs_hibitset_iterator_current_is_set(&it)) {
        cecs_hibitset_iterator_next_set(&it);
    }

    while (!cecs_hibitset_iterator_done(&it)) {
        bool any_unset = false;
        for (size_t i = 0; i < count && !any_unset; i++) {
            any_unset |= !cecs_hibitset_is_set(&bitsets[i], it.current_bit_index);
        }
        if (any_unset) {
            cecs_hibitset_unset(self, a, it.current_bit_index);
        }
        cecs_hibitset_iterator_next_set(&it);
    }
    return self;
}

cecs_hibitset* cecs_hibitset_join(cecs_hibitset* self, const cecs_hibitset* bitsets, size_t count, cecs_arena* a) {
    assert(count >= 1 && "attempted to join less than 2 bitsets");
    cecs_exclusive_range union_range = cecs_hibitset_bit_range(self);
    for (size_t i = 0; i < count; i++) {
        union_range = cecs_exclusive_range_from(
            cecs_range_union(union_range.range, cecs_hibitset_bit_range(&bitsets[i]).range)
        );
    }

    size_t current_bit = union_range.start;
    bool all_done = false;

    while (!all_done) {
        size_t min_next_bit = SIZE_MAX;
        bool any_set = false;
        bool all_done_at_current = true;

        for (size_t i = 0; i < count; i++) {
            cecs_hibitset_iterator j = cecs_hibitset_iterator_create_borrowed_at(&bitsets[i], current_bit);
            size_t next_bit = cecs_hibitset_iterator_next_set(&j);
            min_next_bit = min(min_next_bit, next_bit);

            any_set |= cecs_hibitset_is_set(&bitsets[i], current_bit);
            all_done_at_current &= !cecs_hibitset_bit_in_range(&bitsets[i], min_next_bit);
        }
        if (any_set) {
            cecs_hibitset_set(self, a, current_bit);
        }

        current_bit = min_next_bit;
        all_done = all_done_at_current;
    }
    return self;
}

cecs_hibitset* cecs_hibitset_subtract(cecs_hibitset* self, const cecs_hibitset* subtracted_bitsets, size_t count, cecs_arena* a) {
    assert(count >= 1 && "attempted to subtract less than 2 bitsets");
    cecs_hibitset_iterator it = cecs_hibitset_iterator_create_borrowed_at_first(self);
    if (!cecs_hibitset_iterator_current_is_set(&it)) {
        cecs_hibitset_iterator_next_set(&it);
    }

    while (!cecs_hibitset_iterator_done(&it)) {
        bool any_set = false;
        for (size_t i = 0; i < count && !any_set; i++) {
            any_set |= cecs_hibitset_is_set(&subtracted_bitsets[i], it.current_bit_index);
        }
        if (any_set) {
            cecs_hibitset_unset(self, a, it.current_bit_index);
        }

        cecs_hibitset_iterator_next_set(&it);
    }
    return self;
}
