#include "cecs_iter.h"

cecs_entity cecs_entity_from_storage_sparse_set(
    const cecs_sparse_set_storage *const storage,
    const cecs_entity_storage *const entity_storage,
    const cecs_dense_index index
) {
    const size_t sparse_key = *cecs_sparse_set_get_sparse_key_by_index(&storage->set, index);
    const size_t original_key = cecs_sparse_set_storage_map_key_inverse(storage, sparse_key);
    return cecs_entity_storage_get_used(entity_storage, original_key);
}
extern inline cecs_entity_index cecs_entity_index_from_storage_sparse_set(
    const cecs_sparse_set_storage *const storage,
    const cecs_entity_storage *const entity_storage,
    const cecs_dense_index index
);
