#ifndef CECS_SPARSE_SET_V2_H
#define CECS_SPARSE_SET_V2_H

#include "cecs_allocator.h"
#include "cecs_dynarray.h"
#include <stdint.h>

#ifndef CECS_SPARSE_SET_USIZE_TYPE
#define CECS_SPARSE_SET_USIZE_TYPE_DEFAULT uint32_t
#define CECS_SPARSE_SET_USIZE_TYPE_MAX UINT32_MAX
#define CECS_SPARSE_SET_USIZE_TYPE CECS_SPARSE_SET_USIZE_TYPE_DEFAULT

#else if !defined(CECS_SPARSE_SET_USIZE_TYPE_MAX)
static_assert(
    false,
    "static error: CECS_SPARSE_SET_USIZE_TYPE_MAX must be defined when CECS_SPARSE_SET_USIZE_TYPE is defined"
);

#endif

typedef CECS_SPARSE_SET_USIZE_TYPE cecs_sparse_set_usize;
static_assert(
    ((cecs_sparse_set_usize)(-1) == CECS_SPARSE_SET_USIZE_TYPE_MAX),
    "static error: cecs_sparse_set_usize must be able to hold CECS_SPARSE_SET_USIZE_TYPE_MAX"
);

typedef struct cecs_dense_index {
    cecs_sparse_set_usize value;
} cecs_dense_index;

extern const cecs_dense_index cecs_dense_index_invalid;
inline bool cecs_dense_index_is_valid(const cecs_dense_index index) {
    return index.value != cecs_dense_index_invalid.value;
}
inline cecs_dense_index cecs_dense_index_create_unchecked(const cecs_sparse_set_usize index) {
    return (cecs_dense_index){.value = index};
}
inline cecs_dense_index cecs_dense_index_create_valid(const cecs_sparse_set_usize index) {
    const cecs_dense_index result = cecs_dense_index_create_unchecked(index);
    if (!cecs_dense_index_is_valid(result)) {
        assert(false && "fatal error: cecs_dense_index_create_valid called with invalid index");
        exit(EXIT_FAILURE);
    }
    return result;
}
static inline cecs_dense_index cecs_dense_index_create_invalid(void) {
    return cecs_dense_index_invalid;
}


typedef struct cecs_dense_set {
    cecs_dynarray values;
    cecs_dynarray dense_to_sparse;
} cecs_dense_set;
inline cecs_sparse_set_usize cecs_dense_set_count(const cecs_dense_set *set) {
    return cecs_dynarray_count(&set->values);
}

typedef struct cecs_sparse_set {
    cecs_dense_set values;
    cecs_dynarray sparse_to_dense;
} cecs_sparse_set;

cecs_sparse_set cecs_sparse_set_create(void);
cecs_sparse_set cecs_sparse_set_create_with_capacity(cecs_allocator *allocator, const size_t capacity, const size_t value_size);

inline size_t cecs_sparse_set_value_count(const cecs_sparse_set *set) {
    return cecs_dense_set_count(&set->values);
}
inline size_t cecs_sparse_set_sparse_range_size(const cecs_sparse_set *set) {
    return cecs_dynarray_count(&set->sparse_to_dense);
}
inline const cecs_dense_index *cecs_sparse_set_get_index(const cecs_sparse_set *set, const size_t key) {
    if (key >= cecs_sparse_set_sparse_range_size(set)) {
        assert(false && "fatal error: cecs_sparse_set_index_get called with out of bounds key");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get(&set->sparse_to_dense, key, sizeof(cecs_dense_index));
}
inline cecs_dense_index *cecs_sparse_set_get_index_mut(cecs_sparse_set *set, const size_t key) {
    if (key >= cecs_sparse_set_sparse_range_size(set)) {
        assert(false && "fatal error: cecs_sparse_set_index_get_mut called with out of bounds key");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get_mut(&set->sparse_to_dense, key, sizeof(cecs_dense_index));
}
inline const void *cecs_sparse_set_get_value(const cecs_sparse_set *set, const size_t key, const size_t value_size) {
    const cecs_dense_index index = *(const cecs_dense_index *)cecs_sparse_set_get_index(set, key);
    if (!cecs_dense_index_is_valid(index)) {
        assert(false && "fatal error: cecs_sparse_set_get_value called with invalid index");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get(&set->values.values, index.value, value_size);
}
inline void *cecs_sparse_set_get_value_mut(cecs_sparse_set *set, const size_t key, const size_t value_size) {
    const cecs_dense_index index = *(const cecs_dense_index *)cecs_sparse_set_get_index(set, key);
    if (!cecs_dense_index_is_valid(index)) {
        assert(false && "fatal error: cecs_sparse_set_get_value_mut called with invalid index");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get_mut(&set->values.values, index.value, value_size);
}
inline size_t *cecs_sparse_set_get_sparse_key_mut(cecs_sparse_set *set, const size_t key) {
    const cecs_dense_index index = *(const cecs_dense_index *)cecs_sparse_set_get_index(set, key);
    if (!cecs_dense_index_is_valid(index)) {
        assert(false && "fatal error: cecs_sparse_set_get_sparse_key_mut called with invalid index");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get_mut(&set->values.dense_to_sparse, index.value, sizeof(size_t));
}

inline const void *cecs_sparse_set_get_value_by_index(const cecs_sparse_set *set, const cecs_dense_index index, const size_t value_size) {
    if (!cecs_dense_index_is_valid(index)) {
        assert(false && "fatal error: cecs_sparse_set_get_value_by_index called with invalid index");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get(&set->values.values, index.value, value_size);
}
inline void *cecs_sparse_set_get_value_by_index_mut(cecs_sparse_set *set, const cecs_dense_index index, const size_t value_size) {
    if (!cecs_dense_index_is_valid(index)) {
        assert(false && "fatal error: cecs_sparse_set_get_value_by_index_mut called with invalid index");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get_mut(&set->values.values, index.value, value_size);
}
inline const size_t *cecs_sparse_set_get_sparse_key_by_index(const cecs_sparse_set *set, const cecs_dense_index index) {
    if (!cecs_dense_index_is_valid(index)) {
        assert(false && "fatal error: cecs_sparse_set_get_sparse_key_by_index called with invalid index");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get(&set->values.dense_to_sparse, index.value, sizeof(size_t));
}

void cecs_sparse_set_reserve_sparse_range(cecs_sparse_set *set, cecs_allocator *allocator, const size_t range_size);

void *cecs_sparse_set_insert_within_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size);
void *cecs_sparse_set_insert_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size);
void *cecs_sparse_set_insert(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size);

void cecs_sparse_set_remove_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size);
void cecs_sparse_set_remove(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size);

#endif