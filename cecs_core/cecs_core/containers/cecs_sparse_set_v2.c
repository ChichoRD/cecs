#include "cecs_sparse_set_v2.h"
#include <memory.h>
const cecs_dense_index cecs_dense_index_invalid = {.value = CECS_SPARSE_SET_USIZE_TYPE_MAX};

extern inline bool cecs_dense_index_is_valid(const cecs_dense_index index);
extern inline cecs_sparse_set_usize cecs_dense_set_count(const cecs_dense_set *set);
extern inline cecs_sparse_set_usize cecs_dense_set_capacity(const cecs_dense_set *set);
extern inline cecs_dense_index cecs_dense_index_create_unchecked(const cecs_sparse_set_usize index);
extern inline cecs_dense_index cecs_dense_index_create_valid(const cecs_sparse_set_usize index);
extern inline cecs_dense_index cecs_dense_index_create_invalid(void);

extern inline cecs_sparse_set_usize cecs_sparse_set_value_count(const cecs_sparse_set *set);
extern inline cecs_sparse_set_usize cecs_sparse_set_value_capacity(const cecs_sparse_set *set);
extern inline size_t cecs_sparse_set_sparse_range_size(const cecs_sparse_set *set);

extern inline const cecs_dense_index *cecs_sparse_set_get_index(const cecs_sparse_set *set, const size_t key);
extern inline cecs_dense_index *cecs_sparse_set_get_index_mut(cecs_sparse_set *set, const size_t key);

extern inline bool cecs_sparse_set_contains_within_range(const cecs_sparse_set *set, const size_t key);
extern inline bool cecs_sparse_set_contains(const cecs_sparse_set *set, const size_t key);

extern inline const void *cecs_sparse_set_get_value(const cecs_sparse_set *set, const size_t key, const size_t value_size);
extern inline void *cecs_sparse_set_get_value_mut(cecs_sparse_set *set, const size_t key, const size_t value_size);
extern inline size_t *cecs_sparse_set_get_sparse_key_mut(cecs_sparse_set *set, const size_t key);

extern inline const void *cecs_sparse_set_get_value_by_index(const cecs_sparse_set *set, const cecs_dense_index index, const size_t value_size);
extern inline void *cecs_sparse_set_get_value_by_index_mut(cecs_sparse_set *set, const cecs_dense_index index, const size_t value_size);
extern inline const size_t *cecs_sparse_set_get_sparse_key_by_index(const cecs_sparse_set *set, const cecs_dense_index index);

cecs_dense_set cecs_dense_set_create(void) {
    return (cecs_dense_set){
        .values = cecs_dynarray_create(),
        .sparse_from_dense = cecs_dynarray_create(),
    };
}
cecs_dense_set cecs_dense_set_create_with_capacity(cecs_allocator *allocator, const size_t capacity, const size_t value_size) {
    return (cecs_dense_set){
        .values = cecs_dynarray_create_with_capacity(allocator, capacity, value_size),
        .sparse_from_dense = cecs_dynarray_create_with_capacity(allocator, capacity, sizeof(size_t)),
    };
}
void cecs_dense_set_destroy(cecs_dense_set *set, cecs_allocator *allocator, const size_t value_size) {
    cecs_dynarray_destroy(&set->values, allocator, value_size);
    cecs_dynarray_destroy(&set->sparse_from_dense, allocator, sizeof(size_t));
}


void *cecs_dense_set_push_key(cecs_dense_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size) {
    void *value = cecs_dynarray_push(&set->values, allocator, value_size);
    size_t *sparse_key = cecs_dynarray_push(&set->sparse_from_dense, allocator, sizeof(size_t));
    *sparse_key = key;
    return value;
}
void cecs_dense_set_swap_last_pop(cecs_dense_set *set, cecs_allocator *allocator, const size_t index, const size_t value_size) {
    cecs_dynarray_swap_last_pop(&set->values, allocator, index, value_size);
    cecs_dynarray_swap_last_pop(&set->sparse_from_dense, allocator, index, sizeof(size_t));
}
void cecs_dense_set_clear(cecs_dense_set *set) {
    cecs_dynarray_clear(&set->values);
    cecs_dynarray_clear(&set->sparse_from_dense);
}


cecs_sparse_set cecs_sparse_set_create(void) {
    return (cecs_sparse_set){
        .values = cecs_dense_set_create(),
        .dense_from_sparse = cecs_dynarray_create(),
    };
}
cecs_sparse_set cecs_sparse_set_create_with_capacity(cecs_allocator *allocator, const size_t capacity, const size_t value_size) {
    cecs_sparse_set set = (cecs_sparse_set){
        .values = cecs_dense_set_create_with_capacity(allocator, capacity, value_size),
        .dense_from_sparse = cecs_dynarray_create_with_capacity(allocator, capacity, sizeof(cecs_dense_index)),
    };

    memset(
        cecs_dynarray_push_many(&set.dense_from_sparse, allocator, capacity, sizeof(cecs_dense_index)),
        UINT8_MAX,
        capacity * sizeof(cecs_dense_index)
    );
    return set;
}

void cecs_sparse_set_destroy(cecs_sparse_set *set, cecs_allocator *allocator, const size_t value_size) {
    cecs_dense_set_destroy(&set->values, allocator, value_size);
    cecs_dynarray_destroy(&set->dense_from_sparse, allocator, sizeof(cecs_dense_index));
}

void cecs_sparse_set_reserve_sparse_range(cecs_sparse_set *set, cecs_allocator *allocator, const size_t range_size) {
    const size_t current_size = cecs_sparse_set_sparse_range_size(set);
    if (range_size > current_size) {
        const size_t needed_capacity = range_size - current_size;
        cecs_dynarray_reserve(&set->dense_from_sparse, allocator, range_size, sizeof(cecs_dense_index));
        memset(
            cecs_dynarray_push_many(&set->dense_from_sparse, allocator, needed_capacity, sizeof(cecs_dense_index)),
            UINT8_MAX,
            needed_capacity * sizeof(cecs_dense_index)
        );
    }
}

void *cecs_sparse_set_insert_within_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size) {
    if (key >= cecs_sparse_set_sparse_range_size(set)) {
        assert(false && "fatal error: cecs_sparse_set_insert_within_expect called with out of bounds key");
        exit(EXIT_FAILURE);
    }
    cecs_dense_index *const index = cecs_sparse_set_get_index_mut(set, key);
    if (cecs_dense_index_is_valid(*index)) {
        assert(false && "fatal error: cecs_sparse_set_insert_within_expect called with already occupied key");
        exit(EXIT_FAILURE);
    } else {
        index->value = cecs_dense_set_count(&set->values);
        return cecs_dense_set_push_key(&set->values, allocator, key, value_size);
    }
}
void *cecs_sparse_set_insert_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size){
    cecs_sparse_set_reserve_sparse_range(set, allocator, key + 1);
    return cecs_sparse_set_insert_within_expect(set, allocator, key, value_size);
}
void *cecs_sparse_set_get_or_insert(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size) {
    cecs_sparse_set_reserve_sparse_range(set, allocator, key + 1);
    cecs_dense_index *const index = cecs_sparse_set_get_index_mut(set, key);
    if (cecs_dense_index_is_valid(*index)) {
        return cecs_dynarray_get_mut(&set->values.values, index->value, value_size);
    } else {
        index->value = cecs_dense_set_count(&set->values);
        return cecs_dense_set_push_key(&set->values, allocator, key, value_size);
    }
}

void cecs_sparse_set_remove_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size) {
    if (!cecs_sparse_set_remove(set, allocator, key, value_size)) {
        assert(false && "fatal error: cecs_sparse_set_remove_expect called with non-existent key");
        exit(EXIT_FAILURE);
    }
}
bool cecs_sparse_set_remove(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size) {
    const size_t last_value_key =
        *(const size_t *)cecs_sparse_set_get_sparse_key_by_index(set, cecs_dense_index_create_valid(cecs_sparse_set_value_count(set) - 1));
    cecs_dense_index *const swapped_index = cecs_sparse_set_get_index_mut(set, last_value_key);
    cecs_dense_index *const invalid_index = cecs_sparse_set_get_index_mut(set, key);
    if (cecs_dense_index_is_valid(*invalid_index)) {
        cecs_dense_set_swap_last_pop(&set->values, allocator, invalid_index->value, value_size);
        *swapped_index = *invalid_index;
        *invalid_index = cecs_dense_index_create_invalid();
        return true;
    } else {
        return false;
    }
}

void cecs_sparse_set_clear(cecs_sparse_set *set) {
    cecs_dense_set_clear(&set->values);
    memset(
        cecs_dynarray_first_mut(&set->dense_from_sparse),
        UINT8_MAX,
        cecs_sparse_set_sparse_range_size(set) * sizeof(cecs_dense_index)
    );
}
