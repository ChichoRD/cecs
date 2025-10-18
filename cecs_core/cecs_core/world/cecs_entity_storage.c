#include "cecs_entity_storage.h"

extern inline cecs_entity_storage cecs_entity_storage_create(void);
extern inline cecs_entity_storage cecs_entity_storage_create_with_capacity(cecs_allocator *const allocator, const size_t initial_capacity);
extern inline void cecs_entity_storage_reset(cecs_entity_storage *storage);
extern inline void cecs_entity_storage_destroy(cecs_entity_storage *storage, cecs_allocator *const allocator);

extern inline size_t cecs_entity_storage_total_count(const cecs_entity_storage *storage);
extern inline size_t cecs_entity_storage_free_count(const cecs_entity_storage *storage);
extern inline size_t cecs_entity_storage_used_count(const cecs_entity_storage *storage);

extern inline cecs_entity cecs_entity_storage_peek_entity(const cecs_entity_storage *storage, const size_t index);
cecs_entity cecs_entity_storage_get_entity(const cecs_entity_storage *storage, const size_t index) {
    const cecs_entity entity = cecs_entity_storage_peek_entity(storage, index);
    cecs_assert_or_exit(
        !cecs_entity_is_free(entity),
        "error: cecs_entity_storage_get_entity called for a free entity"
    );
    return entity;
}
cecs_entity cecs_entity_storage_get_entity_exact(const cecs_entity_storage *storage, const cecs_entity entity) {
    const size_t index = cecs_entity_index(entity);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity(storage, index);
    cecs_assert_or_exit(
        stored_entity.value == entity.value,
        "error: cecs_entity_storage_get_entity_exact called with mismatched entity"
    );
    return stored_entity;
}
static inline cecs_entity *cecs_entity_storage_get_entity_exact_mut(cecs_entity_storage *storage, const cecs_entity entity) {
    const size_t index = cecs_entity_index(entity);
    cecs_entity *const stored_entity = (cecs_entity*)cecs_dynarray_get_mut(&storage->entities, index, sizeof(cecs_entity));
    cecs_assert_or_exit(
        !cecs_entity_is_free(*stored_entity),
        "error: cecs_entity_storage_get_entity_exact_mut called for a free entity"
    );
    cecs_assert_or_exit(
        stored_entity->value == entity.value,
        "error: cecs_entity_storage_get_entity_exact_mut called with mismatched entity"
    );
    return stored_entity;
}

cecs_entity cecs_entity_storage_alloc_entity(cecs_entity_storage *storage, cecs_allocator *allocator) {
    if (storage->free_count > 0) {
        const cecs_entity entity = storage->next_free;
        const size_t next_free_index = cecs_entity_index(entity);
        storage->next_free = cecs_entity_storage_peek_entity(storage, next_free_index);
        --storage->free_count;
        return cecs_entity_unset_free(entity);
    } else {
        const size_t new_index = cecs_entity_storage_total_count(storage);
        cecs_entity new_entity = cecs_entity_create(new_index, cecs_entity_meta_type_none, 0);
        cecs_entity *const entity_slot = cecs_dynarray_push(&storage->entities, allocator, sizeof(cecs_entity));
        *entity_slot = new_entity;
        return new_entity;
    }
}
void cecs_entity_storage_free_entity(cecs_entity_storage *storage, const cecs_entity entity) {
    cecs_entity *const entity_slot = cecs_entity_storage_get_entity_exact_mut(storage, entity);
    const cecs_entity free_entity = cecs_entity_set_free(cecs_entity_next_generation(*entity_slot));
    *entity_slot = storage->next_free;
    storage->next_free = free_entity;
    ++storage->free_count;
}
