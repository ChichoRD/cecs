#ifndef CECS_DYNAMIC_ARRAY_H
#define CECS_DYNAMIC_ARRAY_H

#include <stddef.h>
#include <stdint.h>
#include "cecs_arena.h"

#define cecs_padding_between(type, next_type) (offsetof(struct { type _a; next_type _b; }, _b) - sizeof(type))

typedef struct cecs_dynamic_array {
    uint8_t *values;
    size_t count;
    size_t capacity;
} cecs_dynamic_array;

static inline size_t cecs_dynamic_array_count_of_size(const cecs_dynamic_array *l, size_t size) {
    return l->count / size;
}

static inline size_t cecs_dynamic_array_capacity_of_size(const cecs_dynamic_array *l, size_t size) {
    return l->capacity / size;
}

#define CECS_DYNAMIC_ARRAY_COUNT(type, cecs_dynamic_array_ref) cecs_dynamic_array_count_of_size(cecs_dynamic_array_ref, sizeof(type))
#define CECS_DYNAMIC_ARRAY_CAPACITY(type, cecs_dynamic_array_ref) cecs_dynamic_array_capacity_of_size(cecs_dynamic_array_ref, sizeof(type))

cecs_dynamic_array cecs_dynamic_array_create(void);

cecs_dynamic_array cecs_dynamic_array_create_with_capacity(cecs_arena *a, size_t capacity);
#define CECS_DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(type, arena_ref, capacity) cecs_dynamic_array_create_with_capacity(arena_ref, (capacity) * sizeof(type))

void *cecs_dynamic_array_add(cecs_dynamic_array *l, cecs_arena *a, const void *element, size_t size);
#define CECS_DYNAMIC_ARRAY_ADD(type, cecs_dynamic_array_ref, arena_ref, element_ref) \
    ((type *)cecs_dynamic_array_add(cecs_dynamic_array_ref, arena_ref, element_ref, sizeof(type)))

void *cecs_dynamic_array_add_range(cecs_dynamic_array *l, cecs_arena *a, void *elements, size_t count, size_t size);
#define CECS_DYNAMIC_ARRAY_ADD_RANGE(type, cecs_dynamic_array_ref, arena_ref, elements_ref, count) \
    cecs_dynamic_array_add_range(cecs_dynamic_array_ref, arena_ref, elements_ref, count, sizeof(type))

void cecs_dynamic_array_remove(cecs_dynamic_array *l, cecs_arena *a, size_t index, size_t size);
#define CECS_DYNAMIC_ARRAY_REMOVE(type, cecs_dynamic_array_ref, arena_ref, index) \
    cecs_dynamic_array_remove(cecs_dynamic_array_ref, arena_ref, index, sizeof(type))

void cecs_dynamic_array_remove_range(cecs_dynamic_array *l, cecs_arena *a, size_t index, size_t count, size_t size);
#define CECS_DYNAMIC_ARRAY_REMOVE_RANGE(type, dynamic_array_ref, arena_ref, index, count) cecs_dynamic_array_remove_range(dynamic_array_ref, arena_ref, index, count, sizeof(type))

void cecs_dynamic_array_truncate(cecs_dynamic_array *l, cecs_arena *a, const size_t new_count, const size_t size);

void *cecs_dynamic_array_set(cecs_dynamic_array *l, size_t index, void *element, size_t size);
#define CECS_DYNAMIC_ARRAY_SET(type, dynamic_array_ref, index, element_ref) \
    ((type *)cecs_dynamic_array_set(dynamic_array_ref, index, element_ref, sizeof(type)))

static inline void *cecs_dynamic_array_first(const cecs_dynamic_array *l) {
    assert(l->count > 0 && "Attempted to get first element of empty cecs_dynamic_array");
    return l->values;
}

static inline void *cecs_dynamic_array_last(const cecs_dynamic_array *l, size_t size) {
    assert(l->count > 0 && "Attempted to get last element of empty cecs_dynamic_array");
    return (uint8_t *)l->values + (l->count - size);
}
#define CECS_DYNAMIC_ARRAY_LAST(type, dynamic_array_ref) ((type *)cecs_dynamic_array_last(dynamic_array_ref, sizeof(type)))

void *cecs_dynamic_array_remove_swap_last(cecs_dynamic_array *l, cecs_arena *a, size_t index, size_t size);
#define CECS_DYNAMIC_ARRAY_REMOVE_SWAP_LAST(type, dynamic_array_ref, arena_ref, index) ((type *)cecs_dynamic_array_remove_swap_last(dynamic_array_ref, arena_ref, index, sizeof(type)))

void cecs_dynamic_array_clear(cecs_dynamic_array *l);

void *cecs_dynamic_array_get(const cecs_dynamic_array *l, size_t index, size_t size);
#define CECS_DYNAMIC_ARRAY_GET(type, dynamic_array_ref, index) ((type *)cecs_dynamic_array_get(dynamic_array_ref, index, sizeof(type)))

void *cecs_dynamic_array_get_range(const cecs_dynamic_array *l, size_t index, size_t count, size_t size);
#define CECS_DYNAMIC_ARRAY_GET_RANGE(type, dynamic_array_ref, index, count) \
    ((type *)cecs_dynamic_array_get_range(dynamic_array_ref, index, count, sizeof(type)))

void *cecs_dynamic_array_set_range(cecs_dynamic_array *l, size_t index, void *elements, size_t count, size_t size);
#define CECS_DYNAMIC_ARRAY_SET_RANGE(type, dynamic_array_ref, index, elements_ref, count) \
    cecs_dynamic_array_set_range(dynamic_array_ref, index, elements_ref, count, sizeof(type))

void *cecs_dynamic_array_set_copy_range(cecs_dynamic_array *l, size_t index, void *single_src, size_t count, size_t size);
#define CECS_DYNAMIC_ARRAY_SET_COPY_RANGE(type, dynamic_array_ref, index, single_src, count) \
    cecs_dynamic_array_set_copy_range(dynamic_array_ref, index, single_src, count, sizeof(type))

void *cecs_dynamic_array_insert(cecs_dynamic_array *l, cecs_arena *a, size_t index, void *element, size_t size);
#define CECS_DYNAMIC_ARRAY_INSERT(type, dynamic_array_ref, arena_ref, index, element_ref) \
    ((type *)cecs_dynamic_array_insert(dynamic_array_ref, arena_ref, index, element_ref, sizeof(type)))

void *cecs_dynamic_array_insert_range(cecs_dynamic_array *l, cecs_arena *a, size_t index, void *elements, size_t count, size_t size);
#define CECS_DYNAMIC_ARRAY_INSERT_RANGE(type, dynamic_array_ref, arena_ref, index, elements_ref, count) \
    cecs_dynamic_array_insert_range(dynamic_array_ref, arena_ref, index, elements_ref, count, sizeof(type))

void *cecs_dynamic_array_extend(cecs_dynamic_array *l, cecs_arena *a, size_t count, size_t size);
#define CECS_DYNAMIC_ARRAY_APPEND_EMPTY(type, dynamic_array_ref, arena_ref, count) \
    ((type *)cecs_dynamic_array_extend(dynamic_array_ref, arena_ref, count, sizeof(type)))

void *cecs_dynamic_array_extend_within(cecs_dynamic_array *l, cecs_arena *a, size_t index, size_t count, size_t size);
#define CECS_DYNAMIC_ARRAY_PREPEND_EMPTY(type, dynamic_array_ref, arena_ref, index, count) \
    ((type *)cecs_dynamic_array_extend_within(dynamic_array_ref, arena_ref, index, count, sizeof(type)))

#endif