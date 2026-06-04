#ifndef CECS_SPARSE_SET_H
#define CECS_SPARSE_SET_H

#include "cecs_dynarray.h"
#include <cecs_allocator.h>
#include <stdint.h>

#ifndef CECS_SPARSE_SET_USIZE_TYPE
#define CECS_SPARSE_SET_USIZE_TYPE_DEFAULT uint32_t
#define CECS_SPARSE_SET_USIZE_TYPE_MAX UINT32_MAX
#define CECS_SPARSE_SET_USIZE_TYPE CECS_SPARSE_SET_USIZE_TYPE_DEFAULT

#elif !defined(CECS_SPARSE_SET_USIZE_TYPE_MAX)
static_assert(
    false,
    "static error: CECS_SPARSE_SET_USIZE_TYPE_MAX must be defined when CECS_SPARSE_SET_USIZE_TYPE is defined"
);

#endif

// TODO: given that we already enclose indices in `cecs_dense_index`, consider using just size_t, or making `cecs_sparse_set_usize` a size_t by default
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


// typedef struct cecs_sparse_index {
//     cecs_sparse_set_usize value;
// } cecs_sparse_index;
typedef size_t cecs_sparse_index;
typedef struct cecs_dense_set {
    cecs_dynarray values;
    cecs_sparse_index *sparse_from_dense;
} cecs_dense_set;
inline cecs_sparse_set_usize cecs_dense_set_capacity(const cecs_dense_set *set) {
    const size_t values_capacity = cecs_dynarray_capacity(&set->values);
    cecs_debugbreak_fail_unless(
        values_capacity <= (size_t)CECS_SPARSE_SET_USIZE_TYPE_MAX,
        "fatal error: cecs_dense_set_capacity cannot be greater than CECS_SPARSE_SET_USIZE_TYPE_MAX"
    );
    return (cecs_sparse_set_usize)values_capacity;
}
inline cecs_sparse_set_usize cecs_dense_set_count(const cecs_dense_set *set) {
    const size_t values_count = cecs_dynarray_count(&set->values);
    cecs_debugbreak_fail_unless(
        values_count <= (size_t)CECS_SPARSE_SET_USIZE_TYPE_MAX,
        "fatal error: cecs_dense_set_count cannot be greater than CECS_SPARSE_SET_USIZE_TYPE_MAX"
    );
    return (cecs_sparse_set_usize)values_count;
}
inline const cecs_sparse_index *cecs_dense_set_get_sparse_key(const cecs_dense_set *set, const size_t dense_index) {
    cecs_debugbreak_fail_unless(
        dense_index < cecs_dense_set_count(set),
        "fatal error: cecs_dense_set_get_sparse_key called with out of bounds dense_index"
    );
    return set->sparse_from_dense + dense_index;
}
inline cecs_sparse_index *cecs_dense_set_get_sparse_key_mut(cecs_dense_set *set, const size_t dense_index) {
    cecs_debugbreak_fail_unless(
        dense_index < cecs_dense_set_count(set),
        "fatal error: cecs_dense_set_get_sparse_key_mut called with out of bounds dense_index"
    );
    return set->sparse_from_dense + dense_index;
}

typedef struct cecs_sparse_set {
    cecs_dense_set values;
    cecs_dynarray dense_from_sparse;
} cecs_sparse_set;

cecs_sparse_set cecs_sparse_set_create(void);
cecs_sparse_set cecs_sparse_set_create_with_capacity(cecs_allocator *allocator, const size_t capacity, const size_t value_size);
void cecs_sparse_set_destroy(cecs_sparse_set *set, cecs_allocator *allocator, const size_t value_size);

inline cecs_sparse_set_usize cecs_sparse_set_value_capacity(const cecs_sparse_set *set) {
    return cecs_dense_set_capacity(&set->values);
}
inline cecs_sparse_set_usize cecs_sparse_set_value_count(const cecs_sparse_set *set) {
    return cecs_dense_set_count(&set->values);
}
inline size_t cecs_sparse_set_sparse_range_size(const cecs_sparse_set *set) {
    return cecs_dynarray_count(&set->dense_from_sparse);
}
inline const cecs_dense_index *cecs_sparse_set_get_index(const cecs_sparse_set *set, const size_t key) {
    if (key >= cecs_sparse_set_sparse_range_size(set)) {
        assert(false && "fatal error: cecs_sparse_set_index_get called with out of bounds key");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get(&set->dense_from_sparse, key, sizeof(cecs_dense_index));
}
inline cecs_dense_index *cecs_sparse_set_get_index_mut(cecs_sparse_set *set, const size_t key) {
    if (key >= cecs_sparse_set_sparse_range_size(set)) {
        assert(false && "fatal error: cecs_sparse_set_index_get_mut called with out of bounds key");
        exit(EXIT_FAILURE);
    }
    return cecs_dynarray_get_mut(&set->dense_from_sparse, key, sizeof(cecs_dense_index));
}

inline bool cecs_sparse_set_contains_within_range(const cecs_sparse_set *set, const size_t key) {
    const cecs_dense_index *const index = cecs_sparse_set_get_index(set, key);
    return cecs_dense_index_is_valid(*index);
}
inline bool cecs_sparse_set_contains(const cecs_sparse_set *set, const size_t key) {
    return (key < cecs_sparse_set_sparse_range_size(set)) && cecs_sparse_set_contains_within_range(set, key);
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
    return cecs_dense_set_get_sparse_key_mut(&set->values, index.value);
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
    cecs_debugbreak_fail_unless(
        index.value < cecs_dense_set_count(&set->values),
        "fatal error: cecs_sparse_set_get_sparse_key_by_index found index with out of bounds value"
    );
    return cecs_dense_set_get_sparse_key(&set->values, index.value);
}

void cecs_sparse_set_reserve_sparse_range(cecs_sparse_set *set, cecs_allocator *allocator, const size_t range_size);
void cecs_sparse_set_reserve_sparse_range_exact(cecs_sparse_set *set, cecs_allocator *allocator, const size_t range_size);
void cecs_sparse_set_upsize_sparse_range(cecs_sparse_set *set, cecs_allocator *allocator, const size_t range_size);
void cecs_sparse_set_upsize_sparse_range_exact(cecs_sparse_set *set, cecs_allocator *allocator, const size_t range_size);

void *cecs_sparse_set_insert_within_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size);
void *cecs_sparse_set_insert_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size);
void *cecs_sparse_set_get_or_insert(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size);

void cecs_sparse_set_remove_expect(cecs_sparse_set *set, const size_t key, const size_t value_size);
bool cecs_sparse_set_remove(cecs_sparse_set *set, const size_t key, const size_t value_size);
void cecs_sparse_set_clear(cecs_sparse_set *set);

#endif
