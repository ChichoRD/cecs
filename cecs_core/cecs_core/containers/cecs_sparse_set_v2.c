#include "cecs_sparse_set_v2.h"
#include <memory.h>
const cecs_dense_index cecs_sparse_key_invalid = {.value = CECS_SPARSE_SET_USIZE_TYPE_MAX};

extern inline bool cecs_dense_index_is_valid(const cecs_dense_index index);
extern inline cecs_dense_index cecs_dense_index_create_unchecked(const cecs_sparse_set_usize index);
extern inline cecs_dense_index cecs_dense_index_create_valid(const cecs_sparse_set_usize index);
extern inline cecs_dense_index cecs_dense_index_create_invalid(void);


cecs_dense_set cecs_dense_set_create(void) {
    return (cecs_dense_set){
        .values = cecs_dynarray_create(),
        .dense_to_sparse = cecs_dynarray_create(),
    };
}
cecs_dense_set cecs_dense_set_create_with_capacity(cecs_allocator *allocator, const size_t capacity, const size_t value_size) {
    return (cecs_dense_set){
        .values = cecs_dynarray_create_with_capacity(allocator, capacity, value_size),
        .dense_to_sparse = cecs_dynarray_create_with_capacity(allocator, capacity, sizeof(size_t)),
    };
}

inline cecs_sparse_set_usize cecs_dense_set_count(const cecs_dense_set *set) {
    return cecs_dynarray_count(&set->values);
}
void *cecs_dense_set_push_key(cecs_dense_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size) {
    void *value = cecs_dynarray_push(&set->values, allocator, value_size);
    size_t *sparse_key = cecs_dynarray_push(&set->dense_to_sparse, allocator, sizeof(size_t));
    *sparse_key = key;
    return value;
}


cecs_sparse_set cecs_sparse_set_create(void) {
    return (cecs_sparse_set){
        .values = cecs_dense_set_create(),
        .sparse_to_dense = cecs_dynarray_create(),
    };
}
cecs_sparse_set cecs_sparse_set_create_with_capacity(cecs_allocator *allocator, const size_t capacity, const size_t value_size) {
    cecs_sparse_set set = (cecs_sparse_set){
        .values = cecs_dense_set_create_with_capacity(allocator, capacity, value_size),
        .sparse_to_dense = cecs_dynarray_create_with_capacity(allocator, capacity, sizeof(cecs_dense_index)),
    };
    memset(cecs_dynarray_first_mut(&set.sparse_to_dense), UINT8_MAX, capacity * sizeof(cecs_dense_index));
    return set;
}

extern inline size_t cecs_sparse_set_sparse_range_size(const cecs_sparse_set *set);
extern inline const cecs_dense_index *cecs_sparse_set_index_get(const cecs_sparse_set *set, const size_t key);
extern inline cecs_dense_index *cecs_sparse_set_index_get_mut(cecs_sparse_set *set, const size_t key);

void *cecs_sparse_set_insert_within_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size){
    if (key >= cecs_sparse_set_sparse_range_size(set)) {
        assert(false && "fatal error: cecs_sparse_set_insert_within_expect called with out of bounds key");
        exit(EXIT_FAILURE);
    }
    cecs_dense_index *const index = cecs_sparse_set_index_get_mut(set, key);
    if (cecs_dense_index_is_valid(*index)) {
        assert(false && "fatal error: cecs_sparse_set_insert_within_expect called with already occupied key");
        exit(EXIT_FAILURE);
    } else {
        index->value = cecs_dense_set_count(&set->values);
        return cecs_dense_set_push_key(&set->values, allocator, key, value_size);
    }
}
void *cecs_sparse_set_insert_expect(cecs_sparse_set *set, cecs_allocator *allocator, const size_t key, const size_t value_size){
    if (key >= cecs_sparse_set_sparse_range_size(set)) {
        const size_t key_end = key + 1;
        cecs_dynarray_reserve(&set->sparse_to_dense, allocator, key_end, sizeof(cecs_dense_index));
        memset(cecs_dynarray_first_mut(&set->sparse_to_dense) + key, UINT8_MAX, (key_end - key) * sizeof(cecs_dense_index));
        static_assert(false, "TODO ensure method");
    }
    return cecs_sparse_set_insert_within_expect(set, allocator, key, value_size);
}