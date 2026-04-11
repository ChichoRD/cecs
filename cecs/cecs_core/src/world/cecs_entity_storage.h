#ifndef CECS_ENTITY_STORAGE_H
#define CECS_ENTITY_STORAGE_H

#include "cecs_entity.h"
#include <cecs_dynarray.h>
#include <cecs_error.h>

// TODO: add identifier allocator structure to reuse the same structure for component storage, group storage free lists and entity storage free list
typedef struct cecs_entity_storage {
    cecs_dynarray entities;
    cecs_entity next_free;
    size_t free_count;
} cecs_entity_storage;

inline cecs_entity_storage cecs_entity_storage_create(void) {
    return (cecs_entity_storage){
        .entities = cecs_dynarray_create(),
        .next_free = cecs_entity_create(0ull, cecs_entity_meta_type_free, 0),
        .free_count = 0ull,
    };
}
inline cecs_entity_storage cecs_entity_storage_create_with_capacity(cecs_allocator *const allocator, const size_t initial_capacity) {
    return (cecs_entity_storage){
        .entities = cecs_dynarray_create_with_capacity(allocator, initial_capacity, sizeof(cecs_entity)),
        .next_free = cecs_entity_create(0ull, cecs_entity_meta_type_free, 0),
        .free_count = 0ull,
    };
}
static inline void cecs_entity_storage_reset(cecs_entity_storage *storage) {
    storage->free_count = 0ull;
    storage->next_free = cecs_entity_create(0ull, cecs_entity_meta_type_free, 0);
    cecs_dynarray_clear(&storage->entities);
}
inline void cecs_entity_storage_destroy(cecs_entity_storage *storage, cecs_allocator *const allocator) {
    storage->free_count = 0ull;
    storage->next_free = cecs_entity_create(0ull, cecs_entity_meta_type_free, 0);
    cecs_dynarray_destroy(&storage->entities, allocator, sizeof(cecs_entity));
}

inline size_t cecs_entity_storage_total_count(const cecs_entity_storage *storage) {
    return cecs_dynarray_count(&storage->entities);
}
inline size_t cecs_entity_storage_free_count(const cecs_entity_storage *storage) {
    return storage->free_count;
}
inline size_t cecs_entity_storage_used_count(const cecs_entity_storage *storage) {
    return cecs_entity_storage_total_count(storage) - storage->free_count;
}

inline cecs_entity cecs_entity_storage_peek_entity(const cecs_entity_storage *storage, const size_t index) {
    cecs_debugbreak_fail_unless(
        index < cecs_entity_storage_total_count(storage),
        "error: cecs_entity_storage_peek_entity called with out of bounds index"
    );
    return *((const cecs_entity*)cecs_dynarray_get(&storage->entities, index, sizeof(cecs_entity)));
}
cecs_entity cecs_entity_storage_get_entity(const cecs_entity_storage *storage, const size_t index);
// TODO: see if we could have an entity e not necessarily have the same index as entities[e] but only the same generation
cecs_entity cecs_entity_storage_get_entity_exact(const cecs_entity_storage *storage, const cecs_entity entity);

cecs_entity cecs_entity_storage_alloc_entity(cecs_entity_storage *storage, cecs_allocator *allocator);
void cecs_entity_storage_free_entity(cecs_entity_storage *storage, const cecs_entity entity);

#endif
