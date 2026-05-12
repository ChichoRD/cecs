#ifndef CECS_ENTITY_STORAGE_H
#define CECS_ENTITY_STORAGE_H

#include "cecs_entity.h"
#include "registry/cecs_identifier_allocator.h"
#include <cecs_error.h>

typedef struct cecs_entity_storage {
    cecs_identifier_allocator entities;
} cecs_entity_storage;

inline cecs_entity_storage cecs_entity_storage_create(void) {
    return (cecs_entity_storage){
        .entities = cecs_identifier_allocator_create()
    };
}
inline cecs_entity_storage cecs_entity_storage_create_with_capacity(cecs_allocator *const allocator, const size_t initial_capacity) {
    return (cecs_entity_storage){
        .entities = cecs_identifier_allocator_create_with_capacity(allocator, initial_capacity)
    };
}
static inline void cecs_entity_storage_reset(cecs_entity_storage *storage) {
    cecs_identifier_allocator_reset(&storage->entities);
}
inline void cecs_entity_storage_destroy(cecs_entity_storage *storage, cecs_allocator *const allocator) {
    cecs_identifier_allocator_destroy(&storage->entities, allocator);
}

inline size_t cecs_entity_storage_total_count(const cecs_entity_storage *storage) {
    return cecs_identifier_allocator_total_count(&storage->entities);
}
inline size_t cecs_entity_storage_free_count(const cecs_entity_storage *storage) {
    return cecs_identifier_allocator_free_count(&storage->entities);
}
inline size_t cecs_entity_storage_used_count(const cecs_entity_storage *storage) {
    return cecs_identifier_allocator_used_count(&storage->entities);
}

inline cecs_entity cecs_entity_storage_peek(const cecs_entity_storage *storage, const size_t index) {
    return cecs_identifier_allocator_peek(&storage->entities, index);
}
inline cecs_entity cecs_entity_storage_get_free(const cecs_entity_storage *storage, const size_t index) {
    return cecs_identifier_allocator_get_free(&storage->entities, index);
}
inline cecs_entity cecs_entity_storage_get_used(const cecs_entity_storage *storage, const size_t index) {
    return cecs_identifier_allocator_get_used(&storage->entities, index);
}
inline cecs_entity cecs_entity_storage_get_exact(const cecs_entity_storage *storage, const cecs_entity entity) {
    return cecs_identifier_allocator_get_exact(&storage->entities, entity);
}
inline cecs_entity cecs_entity_storage_alloc_entity(cecs_entity_storage *storage, cecs_allocator *allocator) {
    return cecs_identifier_allocator_alloc(&storage->entities, allocator);
}
void cecs_entity_storage_free_entity(cecs_entity_storage *storage, const cecs_entity entity);

#endif
