#ifndef CECS_ITER_H
#define CECS_ITER_H

#include <world/cecs_entity_storage.h>
#include <world/registry/cecs_component_storage.h>

cecs_entity cecs_entity_from_storage_sparse_set(
    const cecs_sparse_set_storage *const storage,
    const cecs_entity_storage *const entity_storage,
    const cecs_dense_index index
);
// cecs_entity cecs_entity_from_storage_*(
//     const cecs_sparse_set_storage *const storage,
//     const cecs_entity_storage *const entity_storage,
//     const cecs_dense_index index
// );
// 
// [...]

inline cecs_entity_index cecs_entity_index_from_storage_sparse_set(
    const cecs_sparse_set_storage *const storage,
    const cecs_entity_storage *const entity_storage,
    const cecs_dense_index index
) {
    const cecs_entity entity = cecs_entity_from_storage_sparse_set(storage, entity_storage, index);
    return cecs_entity_index_of(entity);
}

#endif
