#ifndef CECS_DYNAMIC_ARRAY_H
#define CECS_DYNAMIC_ARRAY_H

#include "cecs_allocator.h"
#include <stddef.h>
#include <stdint.h>

typedef struct cecs_dynarray {
    uint8_t *values;
    size_t values_used;
    size_t values_capacity;
} cecs_dynarray;

inline size_t cecs_dynarray_count(const cecs_dynarray *arr) {
    return arr->values_used;
}
inline size_t cecs_dynarray_capacity(const cecs_dynarray *arr) {
    return arr->values_capacity;
}

inline cecs_dynarray cecs_dynarray_create(void) {
    return (cecs_dynarray) {
        .values = NULL,
        .values_used = 0,
        .values_capacity = 0
    };
}
cecs_dynarray cecs_dynarray_create_with_capacity(cecs_allocator *a, const size_t values_capacity, const size_t value_size);

void cecs_dynarray_reserve_exact(cecs_dynarray *arr, cecs_allocator* a, const size_t values_new_capacity, const size_t value_size);
void cecs_dynarray_reserve(cecs_dynarray *arr, cecs_allocator* a, const size_t values_new_capacity, const size_t value_size);
void cecs_dynarray_shrink_exact(cecs_dynarray *arr, cecs_allocator* a, const size_t values_new_capacity, const size_t value_size);
void cecs_dynarray_shrink(cecs_dynarray *arr, cecs_allocator* a, const size_t values_new_capacity, const size_t value_size);

void *cecs_dynarray_push(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size);
void *cecs_dynarray_push_many(cecs_dynarray *arr, cecs_allocator *a, const size_t count, const size_t value_size);
void *cecs_dynarray_push_many_cpy(cecs_dynarray *arr, cecs_allocator *a, const void *values, const size_t count, const size_t value_size);

void *cecs_dynarray_extend(cecs_dynarray *arr, cecs_allocator *a, const size_t start_index_inclusive, const size_t end_index_exclusive, const size_t value_size);
inline void *cecs_dynarray_insert(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t value_size) {
    return cecs_dynarray_extend(arr, a, index, index + 1, value_size);
}
inline void *cecs_dynarray_insert_many(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t count, const size_t value_size) {
    return cecs_dynarray_extend(arr, a, index, index + count, value_size);
}
void *cecs_dynarray_insert_many_cpy(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const void *values, const size_t count, const size_t value_size);

void cecs_dynarray_pop(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size);
void cecs_dynarray_truncate(cecs_dynarray *arr, cecs_allocator *a, const size_t new_count, const size_t value_size);
void cecs_dynarray_swap_last_pop(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t value_size);

void cecs_dynarray_remove(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t value_size);
void cecs_dynarray_remove_many(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t count, const size_t value_size);
static inline void cecs_dynarray_clear(cecs_dynarray *arr) {
    arr->values_used = 0;
}

void *cecs_dynarray_get_mut(cecs_dynarray *arr, const size_t index, const size_t size);
const void *cecs_dynarray_get(const cecs_dynarray *arr, const size_t index, const size_t size);
void *cecs_dynarray_get_range_mut(cecs_dynarray *arr, const size_t index, const size_t count, const size_t value_size);
const void *cecs_dynarray_get_range(const cecs_dynarray *arr, const size_t index, const size_t count, const size_t value_size);

static inline void *cecs_dynarray_first_mut(cecs_dynarray *arr) {
    if (arr->values_used == 0) {
        assert(false && "error: attempted to get first element of empty cecs_dynarray");
        exit(EXIT_FAILURE);
    }
    return arr->values;
}
static inline const void *cecs_dynarray_first(const cecs_dynarray *arr) {
    if (arr->values_used == 0) {
        assert(false && "error: attempted to get first element of empty cecs_dynarray");
        exit(EXIT_FAILURE);
    }
    return arr->values;
}
static inline void *cecs_dynarray_last_mut(cecs_dynarray *arr, const size_t value_size) {
    if (arr->values_used == 0) {
        assert(false && "error: attempted to get last element of empty cecs_dynarray");
        exit(EXIT_FAILURE);
    }
    return arr->values + ((arr->values_used - 1) * value_size);
}
static inline const void *cecs_dynarray_last(const cecs_dynarray *arr, const size_t value_size) {
    if (arr->values_used == 0) {
        assert(false && "error: attempted to get last element of empty cecs_dynarray");
        exit(EXIT_FAILURE);
    }
    return arr->values + ((arr->values_used - 1) * value_size);
}

#endif