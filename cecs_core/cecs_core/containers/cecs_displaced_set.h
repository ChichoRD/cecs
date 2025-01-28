#ifndef CECS_DISPLACED_SET_H
#define CECS_DISPLACED_SET_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "cecs_union.h"
#include "cecs_dynamic_array.h"
#include "cecs_range.h"

typedef struct cecs_displaced_set {
    cecs_dynamic_array elements;
    cecs_exclusive_range index_range;
} cecs_displaced_set;

cecs_displaced_set cecs_displaced_set_create(void);

cecs_displaced_set cecs_displaced_set_create_with_capacity(cecs_arena *a, size_t capacity);

static inline bool cecs_displaced_set_contains_index(const cecs_displaced_set *s, size_t index) {
    return cecs_exclusive_range_contains(s->index_range, index);
}

static inline size_t cecs_displaced_set_cecs_dynamic_array_index(const cecs_displaced_set *s, size_t index) {
    assert(cecs_displaced_set_contains_index(s, index) && "index out of bounds");
    return index - s->index_range.start;
}

bool cecs_displaced_set_contains(const cecs_displaced_set *s, size_t index, void *null_bit_pattern, size_t size);

bool cecs_displaced_set_is_empty(cecs_displaced_set *s);

void *cecs_displaced_set_expand(cecs_displaced_set *s, cecs_arena *a, size_t index, size_t size, int null_bit_pattern);

void *cecs_displaced_set_set(cecs_displaced_set *s, cecs_arena *a, size_t index, void *element, size_t size);
#define CECS_DISPLACED_SET_SET(type, set_ref, arena_ref, index, element_ref) \
    ((type *)cecs_displaced_set_set(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *cecs_displaced_set_set_range(cecs_displaced_set *s, cecs_arena *a, cecs_inclusive_range range, void *elements, size_t size);
#define CECS_DISPLACED_SET_SET_RANGE(type, set_ref, arena_ref, range, elements_ref) \
    ((type *)cecs_displaced_set_set_range(set_ref, arena_ref, range, elements_ref, sizeof(type)))

void *cecs_displaced_set_set_copy_range(cecs_displaced_set *s, cecs_arena *a, cecs_inclusive_range range, void *single_src, size_t size);
#define CECS_DISPLACED_SET_SET_COPY_RANGE(type, set_ref, arena_ref, range, single_src_ref) \
    ((type *)cecs_displaced_set_set_copy_range(set_ref, arena_ref, range, single_src_ref, sizeof(type)))

void *cecs_displaced_set_get(const cecs_displaced_set *s, size_t index, size_t size);
#define CECS_DISPLACED_SET_GET(type, set_ref, index) \
    ((type *)cecs_displaced_set_get(set_ref, index, sizeof(type)))
    
void *cecs_displaced_set_get_range(const cecs_displaced_set *s, cecs_inclusive_range range, size_t size);
#define CECS_DISPLACED_SET_GET_RANGE(type, set_ref, range) \
    ((type *)cecs_displaced_set_get_range(set_ref, range, sizeof(type)))

bool cecs_displaced_set_remove(cecs_displaced_set *s, size_t index, size_t size, void *null_bit_pattern);

bool cecs_displaced_set_remove_out(cecs_displaced_set *s, size_t index, void *out_removed_element, size_t size, void *null_bit_pattern);

void cecs_displaced_set_clear(cecs_displaced_set *s);

#endif