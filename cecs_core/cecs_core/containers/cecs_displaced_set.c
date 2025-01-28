#include <string.h>
#include <stdlib.h>
#include "cecs_displaced_set.h"

cecs_displaced_set cecs_displaced_set_create(void) {
    return (cecs_displaced_set) { .elements = cecs_dynamic_array_create(), .index_range = { 0, 0 } };
}

cecs_displaced_set cecs_displaced_set_create_with_capacity(cecs_arena* a, size_t capacity) {
    return (cecs_displaced_set) { .elements = cecs_dynamic_array_create_with_capacity(a, capacity), .index_range = { 0, 0 } };
}

bool cecs_displaced_set_contains(const cecs_displaced_set* s, size_t index, void* null_bit_pattern, size_t size) {
    return cecs_displaced_set_contains_index(s, index)
        && (memcmp(cecs_dynamic_array_get(&s->elements, cecs_displaced_set_cecs_dynamic_array_index(s, index), size), null_bit_pattern, size) != 0);
}

bool cecs_displaced_set_is_empty(cecs_displaced_set* s) {
    return cecs_exclusive_range_is_empty(s->index_range);
}

void* cecs_displaced_set_expand(cecs_displaced_set* s, cecs_arena* a, size_t index, size_t size, int null_bit_pattern) {
    if (cecs_displaced_set_is_empty(s)) {
        s->index_range = cecs_exclusive_range_singleton(index);
        return memset(cecs_dynamic_array_append_empty(&s->elements, a, 1, size), null_bit_pattern, size);
    } else if (!cecs_displaced_set_contains_index(s, index)) {
        cecs_exclusive_range expanded_range = cecs_exclusive_range_from(
            cecs_range_union(s->index_range.range, cecs_exclusive_range_singleton(index).range)
        );

        cecs_range_splitting expansion_ranges = cecs_exclusive_range_difference(
            expanded_range,
            s->index_range
        );

        cecs_exclusive_range range0 = cecs_exclusive_range_from(expansion_ranges.ranges[0]);
        cecs_exclusive_range range1 = cecs_exclusive_range_from(expansion_ranges.ranges[1]);
        s->index_range = expanded_range;
        if (!cecs_exclusive_range_is_empty(range0)) {
            size_t missing_count = cecs_exclusive_range_length(range0);
            return memset(cecs_dynamic_array_prepend_empty(&s->elements, a, missing_count, size), null_bit_pattern, missing_count * size);
        } else if (!cecs_exclusive_range_is_empty(range1)) {
            size_t missing_count = cecs_exclusive_range_length(range1);
            return  memset(cecs_dynamic_array_append_empty(&s->elements, a, missing_count, size), null_bit_pattern, missing_count * size);
        } else {
            assert(false && "unreachable: range splitting error");
            exit(EXIT_FAILURE);
        }
    } else {
        size_t array_index = cecs_displaced_set_cecs_dynamic_array_index(s, index);
        assert(
            array_index < cecs_dynamic_array_count_of_size(&s->elements, size)
            && "error: index out of bounds"
        );
        return cecs_dynamic_array_get(&s->elements, array_index, size);
    }
}

void* cecs_displaced_set_set(cecs_displaced_set* s, cecs_arena* a, size_t index, void* element, size_t size) {
    if (!cecs_displaced_set_contains_index(s, index)) {
        cecs_displaced_set_expand(s, a, index, size, 0);
    }
    return cecs_dynamic_array_set(&s->elements, cecs_displaced_set_cecs_dynamic_array_index(s, index), element, size);
}

void *cecs_displaced_set_set_range(cecs_displaced_set *s, cecs_arena *a, cecs_inclusive_range range, void *elements, size_t size) {
    if (!cecs_displaced_set_contains_index(s, range.start)) {
        cecs_displaced_set_expand(s, a, range.start, size, 0);
    }
    if (!cecs_displaced_set_contains_index(s, range.end)) {
        cecs_displaced_set_expand(s, a, range.end, size, 0);
    }

    return cecs_dynamic_array_set_range(
        &s->elements,
        cecs_displaced_set_cecs_dynamic_array_index(s, range.start),
        elements,
        cecs_inclusive_range_length(range),
        size
    );
}

void *cecs_displaced_set_set_copy_range(cecs_displaced_set *s, cecs_arena *a, cecs_inclusive_range range, void *single_src, size_t size) {
    if (!cecs_displaced_set_contains_index(s, range.start)) {
        cecs_displaced_set_expand(s, a, range.start, size, 0);
    }
    if (!cecs_displaced_set_contains_index(s, range.end)) {
        cecs_displaced_set_expand(s, a, range.end, size, 0);
    }

    return cecs_dynamic_array_set_copy_range(
        &s->elements,
        cecs_displaced_set_cecs_dynamic_array_index(s, range.start),
        single_src,
        cecs_inclusive_range_length(range),
        size
    );
}

void* cecs_displaced_set_get(const cecs_displaced_set* s, size_t index, size_t size) {
    assert(cecs_displaced_set_contains_index(s, index) && "index out of bounds");
    return cecs_dynamic_array_get(&s->elements, cecs_displaced_set_cecs_dynamic_array_index(s, index), size);
}

void *cecs_displaced_set_get_range(const cecs_displaced_set *s, cecs_inclusive_range range, size_t size) {
    assert(cecs_displaced_set_contains_index(s, range.start) && "error: range start out of bounds");
    assert(cecs_displaced_set_contains_index(s, range.end) && "error: range end out of bounds");
    return cecs_dynamic_array_get_range(&s->elements, cecs_displaced_set_cecs_dynamic_array_index(s, range.start), cecs_inclusive_range_length(range), size);
}

bool cecs_displaced_set_remove(cecs_displaced_set* s, size_t index, size_t size, void* null_bit_pattern) {
    if (cecs_displaced_set_contains(s, index, null_bit_pattern, size)) {
        return false;
    }
    else {
        memcpy(cecs_displaced_set_get(s, index, size), null_bit_pattern, size);
        return true;
    }
}

bool cecs_displaced_set_remove_out(cecs_displaced_set* s, size_t index, void* out_removed_element, size_t size, void* null_bit_pattern) {
    assert(out_removed_element != NULL && "out_removed_element must not be NULL, use: displaced_set_remove");
    if (cecs_displaced_set_contains(s, index, null_bit_pattern, size)) {
        void* removed = cecs_displaced_set_get(s, index, size);
        memcpy(out_removed_element, removed, size);
        memcpy(removed, null_bit_pattern, size);
        return true;
    }
    else {
        memset(out_removed_element, 0, size);
        return false;
    }
}

void cecs_displaced_set_clear(cecs_displaced_set* s) {
    cecs_dynamic_array_clear(&s->elements);
    s->index_range = (cecs_exclusive_range){ 0, 0 };
}

cecs_counted_set cecs_counted_set_create(void) {
    return (cecs_counted_set) { .elements = cecs_displaced_set_create(), .counts = cecs_displaced_set_create() };
}

cecs_counted_set cecs_counted_set_create_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    return (cecs_counted_set) {
        .elements = cecs_displaced_set_create_with_capacity(a, element_size * element_capacity),
            .counts = cecs_displaced_set_create_with_capacity(a, sizeof(cecs_counted_set_counter) * element_capacity)
    };
}

void* cecs_counted_set_set_many(cecs_counted_set* s, cecs_arena* a, size_t index, void* element, size_t count, size_t size) {
    if (cecs_counted_set_contains(s, index)
        && (memcmp(cecs_displaced_set_get(&s->elements, index, size), element, size)) == 0) {
        *CECS_DISPLACED_SET_GET(cecs_counted_set_counter, &s->counts, index) += (cecs_counted_set_counter)count;
        return element;
    }
    else {
        cecs_displaced_set_set(&s->counts, a, index, &(cecs_counted_set_counter){(cecs_counted_set_counter)count}, sizeof(cecs_counted_set_counter));
        return cecs_displaced_set_set(&s->elements, a, index, element, size);
    }
}

void* cecs_counted_set_set(cecs_counted_set* s, cecs_arena* a, size_t index, void* element, size_t size) {
    return cecs_counted_set_set_many(s, a, index, element, 1, size);
}

void* cecs_counted_set_get(const cecs_counted_set* s, size_t index, size_t size) {
    assert(cecs_counted_set_contains(s, index) && "set does not contain an element with such index");
    return cecs_displaced_set_get(&s->elements, index, size);
}

void* cecs_counted_set_get_or_set(cecs_counted_set* s, cecs_arena* a, size_t index, void* otherwise_element, size_t size) {
    if (cecs_counted_set_contains(s, index)) {
        return cecs_counted_set_get(s, index, size);
    }
    else {
        cecs_displaced_set_set(&s->counts, a, index, &(cecs_counted_set_counter){1}, sizeof(cecs_counted_set_counter));
        return cecs_displaced_set_set(&s->elements, a, index, otherwise_element, size);
    }
}

void* cecs_counted_set_increment_or_set(cecs_counted_set* s, cecs_arena* a, size_t index, void* otherwise_element, size_t size) {
    // FIXME: the if branch was taken, then in the operation cecs_counted_set_get
    // there is an assertion of this same condition, IT FAILED after just being checked
    // IDEA: maybe we have overflowed the counter, and the counter is now 0, very weird???

    // SOL: this operation idea is flawed, because incrementing afterwards checking will cause the count to change and the check to change
    // Therefore this will be removed in the future
    if (cecs_counted_set_contains(s, index)) {
        *CECS_DISPLACED_SET_GET(cecs_counted_set_counter, &s->counts, index) += 1;
        return cecs_counted_set_get(s, index, size);
    }
    else {
        cecs_displaced_set_set(&s->counts, a, index, &(cecs_counted_set_counter){1}, sizeof(cecs_counted_set_counter));
        return cecs_displaced_set_set(&s->elements, a, index, otherwise_element, size);
    }
}

cecs_counted_set_counter cecs_counted_set_remove(cecs_counted_set* s, size_t index) {
    cecs_counted_set_counter count;
    if (!cecs_displaced_set_contains_index(&s->counts, index)
        || ((count = cecs_counted_set_count_of(s, index)) == 0)) {
        return 0;
    }
    else {
        return (*CECS_DISPLACED_SET_GET(cecs_counted_set_counter, &s->counts, index) = count - 1);
    }
}

cecs_counted_set_counter cecs_counted_set_remove_out(cecs_counted_set* s, size_t index, void* out_removed_element, size_t size) {
    cecs_counted_set_counter count;
    assert(out_removed_element != NULL && "out_removed_element must not be NULL, use: counted_set_remove");
    if (!cecs_displaced_set_contains_index(&s->counts, index)
        || ((count = cecs_counted_set_count_of(s, index)) == 0)) {
        memset(out_removed_element, 0, size);
        return 0;
    }
    else {
        *CECS_DISPLACED_SET_GET(cecs_counted_set_counter, &s->counts, index) = count - 1;
        memcpy(out_removed_element, cecs_displaced_set_get(&s->elements, index, size), size);
        return count;
    }
}

bool cecs_counted_set_iterator_done(const cecs_counted_set_iterator* it) {
    return !cecs_displaced_set_contains_index(&CECS_COW_GET_REFERENCE(cecs_counted_set, it->set)->elements, it->current_index);
}

size_t cecs_counted_set_iterator_next(cecs_counted_set_iterator* it) {
    do {
        ++it->current_index;
    } while (
        !cecs_counted_set_iterator_done(it)
        && !cecs_counted_set_contains(CECS_COW_GET_REFERENCE(cecs_counted_set, it->set), it->current_index)
        );
    return it->current_index;
}

void* cecs_counted_set_iterator_current(const cecs_counted_set_iterator* it, size_t size) {
    return cecs_counted_set_get(CECS_COW_GET_REFERENCE(cecs_counted_set, it->set), it->current_index, size);
}

void* cecs_counted_set_iterator_first(cecs_counted_set_iterator* it, size_t size) {
    const cecs_counted_set* set = CECS_COW_GET_REFERENCE(cecs_counted_set, it->set);
    it->current_index = set->elements.index_range.start;
    if (!cecs_counted_set_contains(set, it->current_index))
        cecs_counted_set_iterator_next(it);
    return cecs_counted_set_iterator_current(it, size);
}

size_t cecs_counted_set_iterator_first_index(cecs_counted_set_iterator* it) {
    const cecs_counted_set* set = CECS_COW_GET_REFERENCE(cecs_counted_set, it->set);
    it->current_index = set->elements.index_range.start;
    if (!cecs_counted_set_contains(set, it->current_index))
        cecs_counted_set_iterator_next(it);
    return cecs_counted_set_iterator_current_index(it);
}

cecs_counted_set_iterator cecs_counted_set_iterator_create_borrowed(const cecs_counted_set* set) {
    cecs_counted_set_iterator it = (cecs_counted_set_iterator){
        .set = CECS_COW_CREATE_BORROWED(cecs_counted_set, set),
        .current_index = set->elements.index_range.start
    };
    cecs_counted_set_iterator_first_index(&it);
    return it;
}

cecs_counted_set_iterator cecs_counted_set_iterator_create_owned(const cecs_counted_set set) {
    cecs_counted_set_iterator it = (cecs_counted_set_iterator){
        .set = CECS_COW_CREATE_OWNED(cecs_counted_set, set),
        .current_index = set.elements.index_range.start
    };
    cecs_counted_set_iterator_first_index(&it);
    return it;
}
