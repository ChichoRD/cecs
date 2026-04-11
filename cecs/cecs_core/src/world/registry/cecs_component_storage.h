#ifndef CECS_COMPONENT_STORAGE_H
#define CECS_COMPONENT_STORAGE_H

#include "cecs_component.h"
#include "component_storage/cecs_sparse_set_storage.h"
#include <cecs_allocator.h>

typedef union cecs_internal_component_storage {
    cecs_sparse_set_storage sparse_set;
} cecs_internal_component_storage;
typedef struct cecs_component_storage {
    cecs_internal_component_storage storage;
    cecs_component_storage_type type;
} cecs_component_storage;

cecs_component_storage cecs_component_storage_create_sparse_set(cecs_allocator *const allocator, const size_t component_capacity, const size_t component_size);

void cecs_component_storage_clear(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t component_size);
void cecs_component_storage_destroy(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t component_size);

const void *cecs_component_storage_get(const cecs_component_storage *const storage, const size_t key, const size_t component_size);
void *cecs_component_storage_get_mut(cecs_component_storage *const storage, const size_t key, const size_t component_size);

void *cecs_component_storage_get_or_insert(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t key, const size_t component_size);
void *cecs_component_storage_insert_expect(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t key, const size_t component_size);

bool cecs_component_storage_remove(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t key, const size_t component_size);
void cecs_component_storage_remove_expect(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t key, const size_t component_size);

bool cecs_component_storage_contains(const cecs_component_storage *const storage, const size_t key, const size_t component_size);

inline const cecs_sparse_set_storage *cecs_component_storage_sparse_set(const cecs_component_storage *const storage) {
    cecs_assert_or_exit(
        storage->type == cecs_component_storage_type_sparse_set,
        "fatal error: attempted to access cecs_component_storage as cecs_sparse_set_storage when it is of a different type"
    );
    return &storage->storage.sparse_set;
}
inline cecs_sparse_set_storage *cecs_component_storage_sparse_set_mut(cecs_component_storage *const storage) {
    cecs_assert_or_exit(
        storage->type == cecs_component_storage_type_sparse_set,
        "fatal error: attempted to access cecs_component_storage as cecs_sparse_set_storage when it is of a different type"
    );
    return &storage->storage.sparse_set;
}

#endif
