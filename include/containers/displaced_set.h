#ifndef DISPLACED_SET_H
#define DISPLACED_SET_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "list.h"
#include "range.h"

typedef struct displaced_set {
    list elements;
    exclusive_range index_range;
} displaced_set;

displaced_set displaced_set_create() {
    return (displaced_set){ .elements = list_create(), .index_range = {0, 0} };
}

displaced_set displaced_set_create_with_capacity(arena *a, size_t capacity) {
    return (displaced_set){ .elements = list_create_with_capacity(a, capacity), .index_range = {0, 0} };
}

inline bool displaced_set_contains_index(const displaced_set *s, size_t index) {
    return exclusive_range_contains(s->index_range, index);
}

inline size_t displaced_set_list_index(const displaced_set *s, size_t index) {
    assert(displaced_set_contains_index(s, index) && "index out of bounds");
    return index - s->index_range.start;
}

bool displaced_set_contains(const displaced_set *s, size_t index, void *null_bit_pattern, size_t size) {
    return displaced_set_contains_index(s, index)
        && (memcmp(list_get(&s->elements, displaced_set_list_index(s, index), size), null_bit_pattern, size) != 0);
}

bool displaced_set_is_empty(displaced_set *s) {
    return exclusive_range_is_empty(s->index_range);
}

void *displaced_set_expand(displaced_set *s, arena *a, size_t index, size_t size) {
    if (displaced_set_is_empty(s)) {
        s->index_range = exclusive_range_singleton(index);
        return list_append_empty(&s->elements, a, 1, size);
    } else {
        exclusive_range expanded_range = exclusive_range_from(
            range_union(s->index_range.range, exclusive_range_singleton(index).range)
        );

        range_splitting expansion_ranges = exclusive_range_difference(
            expanded_range,
            s->index_range
        );

        exclusive_range range0 = exclusive_range_from(expansion_ranges.ranges[0]);
        exclusive_range range1 = exclusive_range_from(expansion_ranges.ranges[1]);
        s->index_range = expanded_range;
        if (!exclusive_range_is_empty(range0)) {
            size_t missing_count = exclusive_range_length(range0);
            return list_prepend_empty(&s->elements, a, missing_count, size);
        } else if (!exclusive_range_is_empty(range1)) {
            size_t missing_count = exclusive_range_length(range1);
            return list_append_empty(&s->elements, a, missing_count, size);
        } else {
            assert(false && "unreachable: should not have called expand with an index within bounds");
            return list_get(&s->elements, displaced_set_list_index(s, index), size);
        }
    }
}

void *displaced_set_set(displaced_set *s, arena *a, size_t index, void *element, size_t size) {
    if (!displaced_set_contains_index(s, index)) {
        return memcpy(displaced_set_expand(s, a, index, size), element, size);
    } else {
        return list_set(&s->elements, displaced_set_list_index(s, index), element, size);
    }
}
#define DISPLACED_SET_SET(type, set_ref, arena_ref, index, element_ref) \
    ((type *)displaced_set_set(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *displaced_set_get(const displaced_set *s, size_t index, size_t size) {
    assert(displaced_set_contains_index(s, index) && "index out of bounds");
    return list_get(&s->elements, displaced_set_list_index(s, index), size);
}
#define DISPLACED_SET_GET(type, set_ref, index) \
    ((type *)displaced_set_get(set_ref, index, sizeof(type)))

bool displaced_set_remove(displaced_set *s, size_t index, size_t size, void *null_bit_pattern) {
    if (displaced_set_contains(s, index, null_bit_pattern, size)) {
        return false;
    } else {
        memcpy(displaced_set_get(s, index, size), null_bit_pattern, size);
        return true;
    }
}

bool displaced_set_remove_out(displaced_set *s, size_t index, void *out_removed_element, size_t size, void *null_bit_pattern) {
    assert(out_removed_element != NULL && "out_removed_element must not be NULL, use: displaced_set_remove");
    if (displaced_set_contains(s, index, null_bit_pattern, size)) {
        void *removed = displaced_set_get(s, index, size);
        memcpy(out_removed_element, removed, size);
        memcpy(removed, null_bit_pattern, size);
        return true;
    } else {
        memset(out_removed_element, 0, size);
        return false;
    }
}

void displaced_set_clear(displaced_set *s) {
    list_clear(&s->elements);
    s->index_range = (exclusive_range){0, 0};
}


#define COUNTED_SET_COUNTER_TYPE uint16_t
typedef COUNTED_SET_COUNTER_TYPE counted_set_counter;

typedef struct counted_set {
    displaced_set elements;
    displaced_set counts;
} counted_set;

inline counted_set counted_set_empty() {
    return (counted_set){0};
}

counted_set counted_set_create() {
    return (counted_set){ .elements = displaced_set_create(), .counts = displaced_set_create() };
}

counted_set counted_set_create_with_capacity(arena *a, size_t element_capacity, size_t element_size) {
    return (counted_set){
        .elements = displaced_set_create_with_capacity(a, element_size * element_capacity),
        .counts = displaced_set_create_with_capacity(a, sizeof(counted_set_counter) * element_capacity)
    };
}

inline counted_set_counter counted_set_count_of(const counted_set *s, size_t index) {
    return *DISPLACED_SET_GET(counted_set_counter, &s->counts, index);
}

inline bool counted_set_contains(const counted_set *s, size_t index) {
    return displaced_set_contains_index(&s->counts, index)
        && (counted_set_count_of(s, index) > 0);
}


void *counted_set_set_many(counted_set *s, arena *a, size_t index, void *element, size_t count, size_t size) {
    if (counted_set_contains(s, index)
        && (memcmp(displaced_set_get(&s->elements, index, size), element, size)) == 0) {
        *DISPLACED_SET_GET(counted_set_counter, &s->counts, index) += (counted_set_counter)count;
        return element;
    } else {
        displaced_set_set(&s->counts, a, index, &(counted_set_counter){(counted_set_counter)count}, sizeof(counted_set_counter));
        return displaced_set_set(&s->elements, a, index, element, size);
    }
}

void *counted_set_set(counted_set *s, arena *a, size_t index, void *element, size_t size) {
    return counted_set_set_many(s, a, index, element, 1, size);
}
#define COUNTED_SET_SET(type, set_ref, arena_ref, index, element_ref) \
    ((type *)counted_set_set(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *counted_set_get(const counted_set *s, size_t index, size_t size) {
    assert(counted_set_contains(s, index) && "set does not contain an element with such index");
    return displaced_set_get(&s->elements, index, size);
}
#define COUNTED_SET_GET(type, set_ref, index) \
    ((type *)counted_set_get(set_ref, index, sizeof(type)))

void *counted_set_get_or_set(counted_set *s, arena *a, size_t index, void *otherwise_element, size_t size) {
    if (counted_set_contains(s, index)) {
        return counted_set_get(s, index, size);
    } else {
        displaced_set_set(&s->counts, a, index, &(counted_set_counter){1}, sizeof(counted_set_counter));
        return displaced_set_set(&s->elements, a, index, otherwise_element, size);
    }
}
#define COUNTED_SET_GET_OR_SET(type, set_ref, arena_ref, index, otherwise_element_ref) \
    ((type *)counted_set_get_or_set(set_ref, arena_ref, index, otherwise_element_ref, sizeof(type)))

void *counted_set_increment_or_set(counted_set *s, arena *a, size_t index, void *otherwise_element, size_t size) {
    if (counted_set_contains(s, index)) {
        *DISPLACED_SET_GET(counted_set_counter, &s->counts, index) += 1;
        return counted_set_get(s, index, size);
    } else {
        displaced_set_set(&s->counts, a, index, &(counted_set_counter){1}, sizeof(counted_set_counter));
        return displaced_set_set(&s->elements, a, index, otherwise_element, size);
    }
}
#define COUNTED_SET_INCREMENT_OR_SET(type, set_ref, arena_ref, index, otherwise_element_ref) \
    ((type *)counted_set_increment_or_set(set_ref, arena_ref, index, otherwise_element_ref, sizeof(type)))

counted_set_counter counted_set_remove(counted_set *s, size_t index) {
    counted_set_counter count;
    if (!displaced_set_contains_index(&s->counts, index)
        || ((count = counted_set_count_of(s, index)) == 0)) {
        return 0;
    } else {
        return (*DISPLACED_SET_GET(counted_set_counter, &s->counts, index) = count - 1);
    }
}

counted_set_counter counted_set_remove_out(counted_set *s, size_t index, void *out_removed_element, size_t size) {
    counted_set_counter count;
    assert(out_removed_element != NULL && "out_removed_element must not be NULL, use: counted_set_remove");
    if (!displaced_set_contains_index(&s->counts, index)
        || ((count = counted_set_count_of(s, index)) == 0)) {
        memset(out_removed_element, 0, size);
        return 0;
    } else{
        *DISPLACED_SET_GET(counted_set_counter, &s->counts, index) = count - 1;
        memcpy(out_removed_element, displaced_set_get(&s->elements, index, size), size);
        return count;
    }
}


typedef struct counted_set_iterator {
    const COW_STRUCT(const counted_set, counted_set) set;
    size_t current_index;
} counted_set_iterator;

bool counted_set_iterator_done(const counted_set_iterator *it) {
    return !displaced_set_contains_index(&COW_GET_REFERENCE(counted_set, it->set)->elements, it->current_index);
}

size_t counted_set_iterator_next(counted_set_iterator *it) {
    do {
        ++it->current_index;
    } while (
        !counted_set_iterator_done(it)
        && !counted_set_contains(COW_GET_REFERENCE(counted_set, it->set), it->current_index)
    );
    return it->current_index;
}

void *counted_set_iterator_current(const counted_set_iterator *it, size_t size) {
    return counted_set_get(COW_GET_REFERENCE(counted_set, it->set), it->current_index, size);
}
#define COUNTED_SET_ITERATOR_CURRENT(type, iterator_ref) ((type *)counted_set_iterator_current(iterator_ref, sizeof(type)))

void *counted_set_iterator_first(counted_set_iterator *it, size_t size) {
    const counted_set *set = COW_GET_REFERENCE(counted_set, it->set);
    it->current_index = set->elements.index_range.start;
    if (!counted_set_contains(set, it->current_index))
        counted_set_iterator_next(it);
    return counted_set_iterator_current(it, size);
}

inline size_t counted_set_iterator_current_index(const counted_set_iterator *it) {
    return it->current_index;
}

size_t counted_set_iterator_first_index(counted_set_iterator *it) {
    const counted_set *set = COW_GET_REFERENCE(counted_set, it->set);
    it->current_index = set->elements.index_range.start;
    if (!counted_set_contains(set, it->current_index))
        counted_set_iterator_next(it);
    return counted_set_iterator_current_index(it);
}

counted_set_iterator counted_set_iterator_create_borrowed(const counted_set *set) {
    counted_set_iterator it = (counted_set_iterator){
        .set = COW_CREATE_BORROWED(counted_set, set),
        .current_index = set->elements.index_range.start
    };
    counted_set_iterator_first_index(&it);
    return it;
}

counted_set_iterator counted_set_iterator_create_owned(const counted_set set) {
    counted_set_iterator it = (counted_set_iterator){
        .set = COW_CREATE_OWNED(counted_set, set),
        .current_index = set.elements.index_range.start
    };
    counted_set_iterator_first_index(&it);
    return it;
}

#endif