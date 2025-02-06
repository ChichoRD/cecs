#ifndef CECS_DISPLACED_SET_H
#define CECS_DISPLACED_SET_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "cecs_union.h"
#include "cecs_dynamic_array.h"
#include "cecs_range.h"

typedef struct cecs_sentinel_set {
    cecs_dynamic_array values;
    cecs_exclusive_range index_range;
    cecs_inclusive_range first_last_set;
} cecs_sentinel_set;

cecs_sentinel_set cecs_sentinel_set_create(void);
cecs_sentinel_set cecs_sentinel_set_create_with_capacity(cecs_arena *a, const size_t capacity);

inline bool cecs_sentinel_set_contains_index(const cecs_sentinel_set *s, const size_t index) {
    return cecs_exclusive_range_contains(s->index_range, index);
}

inline bool cecs_sentinel_set_contains_range(const cecs_sentinel_set *s, const cecs_inclusive_range range) {
    return cecs_range_is_subrange(range.range, s->first_last_set.range);
}

bool cecs_sentinel_set_is_empty(const cecs_sentinel_set *s);

void *cecs_sentinel_set_expand_to_include(
    cecs_sentinel_set *s,
    cecs_arena *a,
    const cecs_inclusive_range range,
    const size_t size,
    const uint_fast8_t absent_pattern
);

void *cecs_sentinel_set_set_inbounds(cecs_sentinel_set *s, cecs_arena *a, const size_t index, const void *element, const size_t size);
#define CECS_SENTINEL_SET_SET_INBOUNDS(type, set_ref, arena_ref, index, element_ref) \
    ((type *)cecs_sentinel_set_set_inbounds(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *cecs_sentinel_set_set_range_inbounds(cecs_sentinel_set *s, cecs_arena *a, const cecs_inclusive_range range, const void *elements, const size_t size);
#define CECS_SENTINEL_SET_SET_RANGE_INBOUNDS(type, set_ref, arena_ref, range, elements_ref) \
    ((type *)cecs_sentinel_set_set_range_inbounds(set_ref, arena_ref, range, elements_ref, sizeof(type)))

void *cecs_sentinel_set_set_copy_range_inbounds(cecs_sentinel_set *s, cecs_arena *a, const cecs_inclusive_range range, const void *single_src, const size_t size);
#define CECS_SENTINEL_SET_SET_COPY_RANGE_INBOUNDS(type, set_ref, arena_ref, range, single_src_ref) \
    ((type *)cecs_sentinel_set_set_copy_range_inbounds(set_ref, arena_ref, range, single_src_ref, sizeof(type)))

void *cecs_sentinel_set_get_inbounds(const cecs_sentinel_set *s, const size_t index, const size_t size);
#define CECS_SENTINEL_SET_GET_INBOUNDS(type, set_ref, index) \
    ((type *)cecs_sentinel_set_get_inbounds(set_ref, index, sizeof(type)))
    
void *cecs_sentinel_set_get_range_inbounds(const cecs_sentinel_set *s, const cecs_inclusive_range range, const size_t size);
#define CECS_SENTINEL_SET_GET_RANGE_INBOUNDS(type, set_ref, range) \
    ((type *)cecs_sentinel_set_get_range_inbounds(set_ref, range, sizeof(type)))

bool cecs_sentinel_set_remove(cecs_sentinel_set *s, cecs_arena *a, const size_t index, void *out_removed_element, const size_t size, const uint_fast8_t absent_pattern);
void cecs_sentinel_set_clear(cecs_sentinel_set *s);

#endif