#include "cecs_bitset.h"

cecs_bitset cecs_bitset_create(cecs_arena* a, size_t capacity) {
    cecs_bitset b = (cecs_bitset){
        .bit_words = LIST_CREATE_WITH_CAPACITY(bit_word, a, capacity),
        .word_range = { 0, 0 },
    };
    //bit_word *buffer = calloc(capacity, sizeof(bit_word));
    //LIST_ADD_RANGE(bit_word, &b.bit_words, a, buffer, capacity);
    //free(buffer);
    return b;
}

void cecs_bitset_unset_all(cecs_bitset* b) {
    list_clear(&b->bit_words);
    b->word_range = (exclusive_range){ 0, 0 };
}

cecs_word_range cecs_bitset_expand(cecs_bitset* b, cecs_arena* a, size_t word_index) {
    if (exclusive_range_is_empty(b->word_range)) {
        assert(LIST_COUNT(bit_word, &b->bit_words) == 0);
        LIST_ADD(bit_word, &b->bit_words, a, &((bit_word) { 0 }));
        return (b->word_range = exclusive_range_singleton(word_index));
    }
    cecs_word_range expanded_range = exclusive_range_from(
        range_union(b->word_range.range, exclusive_range_singleton(word_index).range)
    );
    range_splitting expansion_ranges = exclusive_range_difference(
        expanded_range,
        b->word_range
    );

    cecs_word_range range0 = exclusive_range_from(expansion_ranges.ranges[0]);
    cecs_word_range range1 = exclusive_range_from(expansion_ranges.ranges[1]);
    if (!exclusive_range_is_empty(range0)) {
        size_t missing_count = exclusive_range_length(range0);
        bit_word* buffer = calloc(missing_count, sizeof(bit_word));
        LIST_INSERT_RANGE(bit_word, &b->bit_words, a, 0, buffer, missing_count);
        free(buffer);
    }
    else if (!exclusive_range_is_empty(range1)) {
        size_t missing_count = exclusive_range_length(range1);
        bit_word* buffer = calloc(missing_count, sizeof(bit_word));
        LIST_ADD_RANGE(bit_word, &b->bit_words, a, buffer, missing_count);
        free(buffer);
    }

    return (b->word_range = expanded_range);
}

bit_word cecs_bitset_set(cecs_bitset* b, cecs_arena* a, size_t bit_index) {
    size_t word_index = cecs_layer_word_index(bit_index, 0);
    if (!exclusive_range_contains(b->word_range, word_index)) {
        cecs_bitset_expand(b, a, word_index);
    }
    ssize_t list_index = (ssize_t)word_index - b->word_range.start;
    assert(
        list_index >= 0
        && list_index < exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );
    bit_word* w = LIST_GET(bit_word, &b->bit_words, list_index);
    return (*w |= (bit_word)1 << cecs_layer_word_bit_index(bit_index, 0));
}

bit_word cecs_bitset_unset(cecs_bitset* b, cecs_arena* a, size_t bit_index) {
    size_t word_index = cecs_layer_word_index(bit_index, 0);
    if (!exclusive_range_contains(b->word_range, word_index)) {
        cecs_bitset_expand(b, a, word_index);
    }
    ssize_t list_index = (ssize_t)word_index - b->word_range.start;
    assert(
        list_index >= 0
        && list_index < exclusive_range_length(b->word_range)
        && "Index out of bounds, should have expanded or expansion failed"
    );
    bit_word* w = LIST_GET(bit_word, &b->bit_words, list_index);
    return (*w &= ~((bit_word)1 << cecs_layer_word_bit_index(bit_index, 0)));
}

bit_word cecs_bitset_get_word(const cecs_bitset* b, size_t bit_index) {
    size_t word_index = cecs_layer_word_index(bit_index, 0);
    if (!exclusive_range_contains(b->word_range, word_index)) {
        return (bit_word)0;
    }
    ssize_t list_index = (ssize_t)word_index - b->word_range.start;
    bit_word* w = LIST_GET(bit_word, &b->bit_words, list_index);
    return *w;
}

bool cecs_bitset_is_set(const cecs_bitset* b, size_t bit_index) {
    return (cecs_bitset_get_word(b, bit_index)
        & ((bit_word)1 << cecs_layer_word_bit_index(bit_index, 0))) != 0;
}

bool cecs_bitset_bit_in_range(const cecs_bitset* b, size_t bit_index) {
    return exclusive_range_contains(b->word_range, cecs_layer_word_index(bit_index, 0));
}

bitset_iterator cecs_bitset_iterator_create(const cecs_bitset* b) {
    return (bitset_iterator) {
        .bitset = b,
            .current_bit_index = cecs_bit0_from_layer_word_index(b->word_range.start, 0),
    };
}

size_t cecs_bitset_iterator_next_set(bitset_iterator* it) {
    do {
        cecs_bitset_iterator_next(it);
    } while (!cecs_bitset_iterator_done(it) && !cecs_bitset_is_set(it->bitset, it->current_bit_index));
    return it->current_bit_index;
}

size_t cecs_bitset_iterator_next_unset(bitset_iterator* it) {
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
    for (ssize_t layer = CECS_BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = cecs_layer_bit_index(bit_index, layer);
        cecs_bitset_set(&b->bitsets[layer], a, layer_bit);
    }
}

void cecs_hibitset_unset(cecs_hibitset* b, cecs_arena* a, size_t bit_index) {
    for (size_t layer = 0; layer < CECS_BIT_LAYER_COUNT; layer++) {
        size_t layer_bit = cecs_layer_bit_index(bit_index, layer);
        bit_word unset_word = cecs_bitset_unset(&b->bitsets[layer], a, layer_bit);

        size_t layer_word_bit_page = cecs_layer_word_bit_index(bit_index, layer) / CECS_BIT_PAGE_SIZE;
        bit_word unset_word_page = unset_word & cecs_page_mask(layer_word_bit_page);

        if (unset_word_page != 0) {
            break;
        }
    }
}

bool cecs_hibitset_is_set(const cecs_hibitset* b, size_t bit_index) {
    return cecs_bitset_is_set(&b->bitsets[0], bit_index);
}

bool cecs_hibitset_is_set_skip_unset(const cecs_hibitset* b, size_t bit_index, ssize_t* out_unset_bit_skip_count) {
    *out_unset_bit_skip_count = 1;
    for (ssize_t layer = CECS_BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = cecs_layer_bit_index(bit_index, layer);
        bit_word word = cecs_bitset_get_word(&b->bitsets[layer], layer_bit);
        size_t layer_word_bit_shift = layer_bit & (CECS_BIT_WORD_BIT_COUNT - 1);
        bit_word word_shifted_to_bit = word >> layer_word_bit_shift;

        if ((word_shifted_to_bit & (bit_word)1) == (bit_word)0) {
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

bool cecs_hibitset_is_set_skip_unset_reverse(const cecs_hibitset* b, size_t bit_index, ssize_t* out_unset_bit_skip_count) {
    *out_unset_bit_skip_count = 1;
    for (ssize_t layer = CECS_BIT_LAYER_COUNT - 1; layer >= 0; layer--) {
        size_t layer_bit = cecs_layer_bit_index(bit_index, layer);
        bit_word word = cecs_bitset_get_word(&b->bitsets[layer], layer_bit);
        size_t layer_word_bit_shift = layer_bit & (CECS_BIT_WORD_BIT_COUNT - 1);
        bit_word word_shifted_to_bit = word >> layer_word_bit_shift;

        if ((word_shifted_to_bit & (bit_word)1) == (bit_word)0) {
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

bit_word cecs_hibitset_get_word(const cecs_hibitset* b, size_t bit_index) {
    return cecs_bitset_get_word(&b->bitsets[0], bit_index);
}

exclusive_range cecs_hibitset_bit_range(const cecs_hibitset* b) {
    return (exclusive_range) {
        .start = cecs_bit0_from_layer_word_index(b->bitsets[0].word_range.start, 0),
            .end = cecs_bit0_from_layer_word_index(b->bitsets[0].word_range.end, 0),
    };
}

bool cecs_hibitset_bit_in_range(const cecs_hibitset* b, size_t bit_index) {
    return cecs_bitset_bit_in_range(&b->bitsets[0], bit_index);
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at(const cecs_hibitset* b, size_t bit_index) {
    return (cecs_hibitset_iterator) {
        .hibitset = COW_CREATE_BORROWED(cecs_hibitset, b),
            .current_bit_index = bit_index,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at_first(const cecs_hibitset* b) {
    return (cecs_hibitset_iterator) {
        .hibitset = COW_CREATE_BORROWED(cecs_hibitset, b),
            .current_bit_index = cecs_hibitset_bit_range(b).start,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_borrowed_at_last(const cecs_hibitset* b) {
    return (cecs_hibitset_iterator) {
        .hibitset = COW_CREATE_BORROWED(cecs_hibitset, b),
            .current_bit_index = cecs_hibitset_bit_range(b).end - 1,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at(const cecs_hibitset b, size_t bit_index) {
    return (cecs_hibitset_iterator) {
        .hibitset = COW_CREATE_OWNED(cecs_hibitset, b),
            .current_bit_index = bit_index,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at_first(const cecs_hibitset b) {
    return (cecs_hibitset_iterator) {
        .hibitset = COW_CREATE_OWNED(cecs_hibitset, b),
            .current_bit_index = cecs_hibitset_bit_range(&b).start,
    };
}

cecs_hibitset_iterator cecs_hibitset_iterator_create_owned_at_last(const cecs_hibitset b) {
    return (cecs_hibitset_iterator) {
        .hibitset = COW_CREATE_OWNED(cecs_hibitset, b),
            .current_bit_index = cecs_hibitset_bit_range(&b).end - 1,
    };
}

size_t cecs_hibitset_iterator_next_set(cecs_hibitset_iterator* it) {
    ssize_t unset_bit_skip_count = 1;
    do {
        it->current_bit_index += unset_bit_skip_count;
    } while (!cecs_hibitset_iterator_done(it)
        && !cecs_hibitset_is_set_skip_unset(COW_GET_REFERENCE(cecs_hibitset, it->hibitset), it->current_bit_index, &unset_bit_skip_count));
    return it->current_bit_index;
}

size_t cecs_hibitset_iterator_previous_set(cecs_hibitset_iterator* it) {
    ssize_t unset_bit_skip_count = 1;
    do {
        it->current_bit_index -= unset_bit_skip_count;
    } while (!cecs_hibitset_iterator_done(it)
        && !cecs_hibitset_is_set_skip_unset_reverse(COW_GET_REFERENCE(cecs_hibitset, it->hibitset), it->current_bit_index, &unset_bit_skip_count)
        && (ssize_t)it->current_bit_index >= unset_bit_skip_count);
    return it->current_bit_index;
}

// TODO: test hibitset operations onto, result mutates parameter
cecs_hibitset cecs_hibitset_intersection(const cecs_hibitset* bitsets, size_t count, cecs_arena* a) {
    assert(count >= 2 && "attempted to compute intersection of less than 2 bitsets");
    exclusive_range intersection_range = cecs_hibitset_bit_range(&bitsets[0]);
    for (size_t i = 1; i < count; i++) {
        intersection_range = exclusive_range_from(
            range_intersection(intersection_range.range, cecs_hibitset_bit_range(&bitsets[i]).range)
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
    exclusive_range union_range = cecs_hibitset_bit_range(&bitsets[0]);
    for (size_t i = 1; i < count; i++) {
        union_range = exclusive_range_from(
            range_union(union_range.range, cecs_hibitset_bit_range(&bitsets[i]).range)
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
    exclusive_range union_range = cecs_hibitset_bit_range(self);
    for (size_t i = 0; i < count; i++) {
        union_range = exclusive_range_from(
            range_union(union_range.range, cecs_hibitset_bit_range(&bitsets[i]).range)
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
