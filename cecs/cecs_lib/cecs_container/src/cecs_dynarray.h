#ifndef CECS_DYNARRAY_H
#define CECS_DYNARRAY_H

#include <cecs_allocator.h>
#include <stddef.h>
#include <stdint.h>

typedef struct cecs_array {
    unsigned char *values;
    size_t values_used;
    size_t values_capacity;
} cecs_array;

inline size_t cecs_array_count(const cecs_array *arr) {
    return arr->values_used;
}
inline size_t cecs_array_capacity(const cecs_array *arr) {
    return arr->values_capacity;
}

inline cecs_array cecs_array_create(void *const values, const size_t capacity) {
    cecs_debugbreak_fail_unless(
        values != NULL || capacity == 0,
        "error: attempted to create cecs_array with non-null values pointer but zero capacity"
    );
    return (cecs_array) {
        .values = (unsigned char *)values,
        .values_used = 0,
        .values_capacity = capacity
    };
}

void cecs_array_reserve_exact(cecs_array *arr, const size_t additional_capacity);

void *cecs_array_push(cecs_array *arr, const size_t value_size);
void *cecs_array_push_many(cecs_array *arr, const size_t count, const size_t value_size);
void *cecs_array_push_many_copy(cecs_array *arr, const void *values, const size_t count, const size_t value_size);

void *cecs_array_extend(cecs_array *arr, const size_t start_index_inclusive, const size_t end_index_exclusive, const size_t value_size);
void *cecs_array_insert(cecs_array *arr, const size_t index, const size_t value_size);
void *cecs_array_insert_many(cecs_array *arr, const size_t index, const size_t count, const size_t value_size);
void *cecs_array_insert_many_copy(cecs_array *arr, const size_t index, const void *values, const size_t count, const size_t value_size);

void cecs_array_pop(cecs_array *arr);
void cecs_array_truncate(cecs_array *arr, const size_t new_count);
void cecs_array_swap_last_pop(cecs_array *arr, const size_t index, const size_t value_size);

void cecs_array_remove(cecs_array *arr, const size_t index, const size_t value_size);
void cecs_array_remove_many(cecs_array *arr, const size_t index, const size_t count, const size_t value_size);
static inline void cecs_array_clear(cecs_array *arr) {
    arr->values_used = 0;
}

const void *cecs_array_get(const cecs_array *arr, const size_t index, const size_t size);
void *cecs_array_get_mut(cecs_array *arr, const size_t index, const size_t size);
const void *cecs_array_get_range(const cecs_array *arr, const size_t index, const size_t count, const size_t value_size);
void *cecs_array_get_range_mut(cecs_array *arr, const size_t index, const size_t count, const size_t value_size);

static inline const void *cecs_array_first(const cecs_array *arr) {
    cecs_debugbreak_fail_unless(
        arr->values_used > 0,
        "error: attempted to get first element of empty cecs_array"
    );
    return arr->values;
}
static inline void *cecs_array_first_mut(cecs_array *arr) {
    cecs_debugbreak_fail_unless(
        arr->values_used > 0,
        "error: attempted to get first element of empty cecs_array"
    );
    return arr->values;
}
static inline const void *cecs_array_last(const cecs_array *arr, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        arr->values_used > 0,
        "error: attempted to get last element of empty cecs_array"
    );
    return arr->values + ((arr->values_used - 1) * value_size);
}
static inline void *cecs_array_last_mut(cecs_array *arr, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        arr->values_used > 0,
        "error: attempted to get last element of empty cecs_array"
    );
    return arr->values + ((arr->values_used - 1) * value_size);
}



typedef struct cecs_dynarray {
    cecs_array array;
} cecs_dynarray;

inline size_t cecs_dynarray_count(const cecs_dynarray *arr) {
    return cecs_array_count(&arr->array);
}
inline size_t cecs_dynarray_capacity(const cecs_dynarray *arr) {
    return cecs_array_capacity(&arr->array);
}

inline cecs_dynarray cecs_dynarray_create(void) {
    return (cecs_dynarray) {
        .array = cecs_array_create(NULL, 0)
    };
}
inline cecs_dynarray cecs_dynarray_create_from_parts(void *const values, const size_t values_used, const size_t values_capacity) {
    cecs_debugbreak_fail_unless(
        values_used <= values_capacity,
        "error: attempted to create cecs_dynarray with values_used greater than values_capacity"
    );
    cecs_debugbreak_fail_unless(
        values != NULL || values_capacity == 0,
        "error: attempted to create cecs_dynarray with non-null values pointer but zero values_capacity"
    );
    return (cecs_dynarray) {
        .array = (cecs_array) {
            .values = (unsigned char *)values,
            .values_used = values_used,
            .values_capacity = values_capacity
        }
    };
}
cecs_dynarray cecs_dynarray_create_with_capacity(cecs_allocator *a, const size_t values_capacity, const size_t value_size);
void cecs_dynarray_destroy(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size);

void cecs_dynarray_reserve_exact(cecs_dynarray *arr, cecs_allocator* a, const size_t additional_capacity, const size_t value_size);
void cecs_dynarray_reserve(cecs_dynarray *arr, cecs_allocator* a, const size_t additional_capacity, const size_t value_size);
void cecs_dynarray_shrink_exact(cecs_dynarray *arr, cecs_allocator* a, const size_t values_new_capacity, const size_t value_size);
void cecs_dynarray_shrink(cecs_dynarray *arr, cecs_allocator* a, const size_t values_new_capacity, const size_t value_size);
void cecs_dynarray_shrink_to_fit(cecs_dynarray *arr, cecs_allocator* a, const size_t value_size);

void *cecs_dynarray_push(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size);
void *cecs_dynarray_push_many(cecs_dynarray *arr, cecs_allocator *a, const size_t count, const size_t value_size);
void *cecs_dynarray_push_many_copy(cecs_dynarray *arr, cecs_allocator *a, const void *values, const size_t count, const size_t value_size);

void *cecs_dynarray_extend(cecs_dynarray *arr, cecs_allocator *a, const size_t start_index_inclusive, const size_t end_index_exclusive, const size_t value_size);
void *cecs_dynarray_insert(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t value_size);
void *cecs_dynarray_insert_many(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t count, const size_t value_size);
void *cecs_dynarray_insert_many_copy(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const void *values, const size_t count, const size_t value_size);

void cecs_dynarray_pop(cecs_dynarray *arr);
void cecs_dynarray_truncate(cecs_dynarray *arr, const size_t new_count);
void cecs_dynarray_swap_last_pop(cecs_dynarray *arr, const size_t index, const size_t value_size);

void cecs_dynarray_remove(cecs_dynarray *arr, const size_t index, const size_t value_size);
void cecs_dynarray_remove_many(cecs_dynarray *arr, const size_t index, const size_t count, const size_t value_size);
static inline void cecs_dynarray_clear(cecs_dynarray *arr) {
    cecs_array_clear(&arr->array);
}

void *cecs_dynarray_get_mut(cecs_dynarray *arr, const size_t index, const size_t size);
const void *cecs_dynarray_get(const cecs_dynarray *arr, const size_t index, const size_t size);
void *cecs_dynarray_get_range_mut(cecs_dynarray *arr, const size_t index, const size_t count, const size_t value_size);
const void *cecs_dynarray_get_range(const cecs_dynarray *arr, const size_t index, const size_t count, const size_t value_size);

static inline void *cecs_dynarray_first_mut(cecs_dynarray *arr) {
    return cecs_array_first_mut(&arr->array);
}
static inline const void *cecs_dynarray_first(const cecs_dynarray *arr) {
    return cecs_array_first(&arr->array);
}
static inline void *cecs_dynarray_last_mut(cecs_dynarray *arr, const size_t value_size) {
    return cecs_array_last_mut(&arr->array, value_size);
}
static inline const void *cecs_dynarray_last(const cecs_dynarray *arr, const size_t value_size) {
    return cecs_array_last(&arr->array, value_size);
}

#endif
