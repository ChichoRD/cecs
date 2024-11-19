#ifndef DISPLACED_SET_H
#define DISPLACED_SET_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "tagged_union.h"
#include "list.h"
#include "range.h"

typedef struct displaced_set {
    list elements;
    exclusive_range index_range;
} displaced_set;

displaced_set displaced_set_create();

displaced_set displaced_set_create_with_capacity(arena *a, size_t capacity);

inline bool displaced_set_contains_index(const displaced_set *s, size_t index) {
    return exclusive_range_contains(s->index_range, index);
}

inline size_t displaced_set_list_index(const displaced_set *s, size_t index) {
    assert(displaced_set_contains_index(s, index) && "index out of bounds");
    return index - s->index_range.start;
}

bool displaced_set_contains(const displaced_set *s, size_t index, void *null_bit_pattern, size_t size);

bool displaced_set_is_empty(displaced_set *s);

void *displaced_set_expand(displaced_set *s, arena *a, size_t index, size_t size);

void *displaced_set_set(displaced_set *s, arena *a, size_t index, void *element, size_t size);
#define DISPLACED_SET_SET(type, set_ref, arena_ref, index, element_ref) \
    ((type *)displaced_set_set(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *displaced_set_get(const displaced_set *s, size_t index, size_t size);
#define DISPLACED_SET_GET(type, set_ref, index) \
    ((type *)displaced_set_get(set_ref, index, sizeof(type)))

bool displaced_set_remove(displaced_set *s, size_t index, size_t size, void *null_bit_pattern);

bool displaced_set_remove_out(displaced_set *s, size_t index, void *out_removed_element, size_t size, void *null_bit_pattern);

void displaced_set_clear(displaced_set *s);


#define COUNTED_SET_COUNTER_TYPE uint16_t
typedef COUNTED_SET_COUNTER_TYPE counted_set_counter;

typedef struct counted_set {
    displaced_set elements;
    displaced_set counts;
} counted_set;

inline counted_set counted_set_empty() {
    return (counted_set){0};
}

counted_set counted_set_create();

counted_set counted_set_create_with_capacity(arena *a, size_t element_capacity, size_t element_size);

inline counted_set_counter counted_set_count_of(const counted_set *s, size_t index) {
    return *DISPLACED_SET_GET(counted_set_counter, &s->counts, index);
}

inline bool counted_set_contains(const counted_set *s, size_t index) {
    return displaced_set_contains_index(&s->counts, index)
        && (counted_set_count_of(s, index) > 0);
}


void *counted_set_set_many(counted_set *s, arena *a, size_t index, void *element, size_t count, size_t size);

void *counted_set_set(counted_set *s, arena *a, size_t index, void *element, size_t size);
#define COUNTED_SET_SET(type, set_ref, arena_ref, index, element_ref) \
    ((type *)counted_set_set(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *counted_set_get(const counted_set *s, size_t index, size_t size);
#define COUNTED_SET_GET(type, set_ref, index) \
    ((type *)counted_set_get(set_ref, index, sizeof(type)))

void *counted_set_get_or_set(counted_set *s, arena *a, size_t index, void *otherwise_element, size_t size);
#define COUNTED_SET_GET_OR_SET(type, set_ref, arena_ref, index, otherwise_element_ref) \
    ((type *)counted_set_get_or_set(set_ref, arena_ref, index, otherwise_element_ref, sizeof(type)))

void *counted_set_increment_or_set(counted_set *s, arena *a, size_t index, void *otherwise_element, size_t size);
#define COUNTED_SET_INCREMENT_OR_SET(type, set_ref, arena_ref, index, otherwise_element_ref) \
    ((type *)counted_set_increment_or_set(set_ref, arena_ref, index, otherwise_element_ref, sizeof(type)))

counted_set_counter counted_set_remove(counted_set *s, size_t index);

counted_set_counter counted_set_remove_out(counted_set *s, size_t index, void *out_removed_element, size_t size);


typedef struct counted_set_iterator {
    const COW_STRUCT(const counted_set, counted_set) set;
    size_t current_index;
} counted_set_iterator;

bool counted_set_iterator_done(const counted_set_iterator *it);

size_t counted_set_iterator_next(counted_set_iterator *it);

void *counted_set_iterator_current(const counted_set_iterator *it, size_t size);
#define COUNTED_SET_ITERATOR_CURRENT(type, iterator_ref) ((type *)counted_set_iterator_current(iterator_ref, sizeof(type)))

void *counted_set_iterator_first(counted_set_iterator *it, size_t size);

inline size_t counted_set_iterator_current_index(const counted_set_iterator *it) {
    return it->current_index;
}

size_t counted_set_iterator_first_index(counted_set_iterator *it);

counted_set_iterator counted_set_iterator_create_borrowed(const counted_set *set);

counted_set_iterator counted_set_iterator_create_owned(const counted_set set);

#endif