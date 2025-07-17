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
        .dense_to_sparse = cecs_dynarray_create_with_capacity(allocator, capacity, sizeof(cecs_dense_index)),
    };
}

inline cecs_sparse_set_usize cecs_dense_set_count(const cecs_dense_set *set) {
    return cecs_dynarray_count(&set->values);
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