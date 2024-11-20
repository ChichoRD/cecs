#ifndef CECS_DISPLACED_SET_H
#define CECS_DISPLACED_SET_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "cecs_union.h"
#include "cecs_dynamic_array.h"
#include "cecs_range.h"

typedef struct cecs_displaced_set {
    cecs_dynamic_array elements;
    cecs_exclusive_range index_range;
} cecs_displaced_set;

cecs_displaced_set cecs_displaced_set_create(void);

cecs_displaced_set cecs_displaced_set_create_with_capacity(cecs_arena *a, size_t capacity);

inline bool cecs_displaced_set_contains_index(const cecs_displaced_set *s, size_t index) {
    return cecs_exclusive_range_contains(s->index_range, index);
}

inline size_t cecs_displaced_set_cecs_dynamic_array_index(const cecs_displaced_set *s, size_t index) {
    assert(cecs_displaced_set_contains_index(s, index) && "index out of bounds");
    return index - s->index_range.start;
}

bool cecs_displaced_set_contains(const cecs_displaced_set *s, size_t index, void *null_bit_pattern, size_t size);

bool cecs_displaced_set_is_empty(cecs_displaced_set *s);

void *cecs_displaced_set_expand(cecs_displaced_set *s, cecs_arena *a, size_t index, size_t size);

void *cecs_displaced_set_set(cecs_displaced_set *s, cecs_arena *a, size_t index, void *element, size_t size);
#define CECS_DISPLACED_SET_SET(type, set_ref, arena_ref, index, element_ref) \
    ((type *)cecs_displaced_set_set(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *cecs_displaced_set_get(const cecs_displaced_set *s, size_t index, size_t size);
#define CECS_DISPLACED_SET_GET(type, set_ref, index) \
    ((type *)cecs_displaced_set_get(set_ref, index, sizeof(type)))

bool cecs_displaced_set_remove(cecs_displaced_set *s, size_t index, size_t size, void *null_bit_pattern);

bool cecs_displaced_set_remove_out(cecs_displaced_set *s, size_t index, void *out_removed_element, size_t size, void *null_bit_pattern);

void cecs_displaced_set_clear(cecs_displaced_set *s);


#define CECS_COUNTED_SET_COUNTER_TYPE uint16_t
typedef CECS_COUNTED_SET_COUNTER_TYPE cecs_counted_set_counter;

typedef struct cecs_counted_set {
    cecs_displaced_set elements;
    cecs_displaced_set counts;
} cecs_counted_set;

inline cecs_counted_set cecs_counted_set_empty(void) {
    return (cecs_counted_set){0};
}

cecs_counted_set cecs_counted_set_create(void);

cecs_counted_set cecs_counted_set_create_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);

inline cecs_counted_set_counter cecs_counted_set_count_of(const cecs_counted_set *s, size_t index) {
    return *CECS_DISPLACED_SET_GET(cecs_counted_set_counter, &s->counts, index);
}

inline bool cecs_counted_set_contains(const cecs_counted_set *s, size_t index) {
    return cecs_displaced_set_contains_index(&s->counts, index)
        && (cecs_counted_set_count_of(s, index) > 0);
}


void *cecs_counted_set_set_many(cecs_counted_set *s, cecs_arena *a, size_t index, void *element, size_t count, size_t size);

void *cecs_counted_set_set(cecs_counted_set *s, cecs_arena *a, size_t index, void *element, size_t size);
#define CECS_COUNTED_SET_SET(type, set_ref, arena_ref, index, element_ref) \
    ((type *)cecs_counted_set_set(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *cecs_counted_set_get(const cecs_counted_set *s, size_t index, size_t size);
#define CECS_COUNTED_SET_GET(type, set_ref, index) \
    ((type *)cecs_counted_set_get(set_ref, index, sizeof(type)))

void *cecs_counted_set_get_or_set(cecs_counted_set *s, cecs_arena *a, size_t index, void *otherwise_element, size_t size);
#define CECS_COUNTED_SET_GET_OR_SET(type, set_ref, arena_ref, index, otherwise_element_ref) \
    ((type *)cecs_counted_set_get_or_set(set_ref, arena_ref, index, otherwise_element_ref, sizeof(type)))

void *cecs_counted_set_increment_or_set(cecs_counted_set *s, cecs_arena *a, size_t index, void *otherwise_element, size_t size);
#define CECS_COUNTED_SET_INCREMENT_OR_SET(type, set_ref, arena_ref, index, otherwise_element_ref) \
    ((type *)cecs_counted_set_increment_or_set(set_ref, arena_ref, index, otherwise_element_ref, sizeof(type)))

cecs_counted_set_counter cecs_counted_set_remove(cecs_counted_set *s, size_t index);

cecs_counted_set_counter cecs_counted_set_remove_out(cecs_counted_set *s, size_t index, void *out_removed_element, size_t size);


typedef struct cecs_counted_set_iterator {
    const CECS_COW_STRUCT(const cecs_counted_set, cecs_counted_set) set;
    size_t current_index;
} cecs_counted_set_iterator;

bool cecs_counted_set_iterator_done(const cecs_counted_set_iterator *it);

size_t cecs_counted_set_iterator_next(cecs_counted_set_iterator *it);

void *cecs_counted_set_iterator_current(const cecs_counted_set_iterator *it, size_t size);
#define COUNTED_SET_ITERATOR_CURRENT(type, iterator_ref) ((type *)cecs_counted_set_iterator_current(iterator_ref, sizeof(type)))

void *cecs_counted_set_iterator_first(cecs_counted_set_iterator *it, size_t size);

inline size_t cecs_counted_set_iterator_current_index(const cecs_counted_set_iterator *it) {
    return it->current_index;
}

size_t cecs_counted_set_iterator_first_index(cecs_counted_set_iterator *it);

cecs_counted_set_iterator cecs_counted_set_iterator_create_borrowed(const cecs_counted_set *set);

cecs_counted_set_iterator cecs_counted_set_iterator_create_owned(const cecs_counted_set set);

#endif