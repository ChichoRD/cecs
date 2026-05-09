#include "cecs_entity_storage.h"
#include <cecs_type_traits.h>

static_assert(
    CECS_IS_SAME_TYPE(cecs_entity, cecs_identifier),
    "static error: cecs_entity and cecs_identifier must be the same type for cecs_entity_storage to work correctly"
);

extern inline cecs_entity_storage cecs_entity_storage_create(void);
extern inline cecs_entity_storage cecs_entity_storage_create_with_capacity(cecs_allocator *const allocator, const size_t initial_capacity);
extern inline void cecs_entity_storage_reset(cecs_entity_storage *storage);
extern inline void cecs_entity_storage_destroy(cecs_entity_storage *storage, cecs_allocator *const allocator);

extern inline size_t cecs_entity_storage_total_count(const cecs_entity_storage *storage);
extern inline size_t cecs_entity_storage_free_count(const cecs_entity_storage *storage);
extern inline size_t cecs_entity_storage_used_count(const cecs_entity_storage *storage);

extern inline cecs_entity cecs_entity_storage_peek(const cecs_entity_storage *storage, const size_t index);
extern inline cecs_entity cecs_entity_storage_get_used(const cecs_entity_storage *storage, const size_t index);
extern inline cecs_entity cecs_entity_storage_get_free(const cecs_entity_storage *storage, const size_t index);
extern inline cecs_entity cecs_entity_storage_get_exact(const cecs_entity_storage *storage, const cecs_entity entity);

// static inline cecs_entity *cecs_entity_storage_peek_entity_mut(cecs_entity_storage *storage, const size_t index) {
//     return (cecs_entity *)cecs_identifier_allocator_peek_mut(&storage->entities, index);
// }
extern cecs_identifier *cecs_identifier_allocator_get_used_mut(cecs_identifier_allocator *const storage, const size_t index);
extern inline cecs_entity *cecs_entity_storage_get_used_mut(cecs_entity_storage *storage, const size_t index) {
    return (cecs_entity *)cecs_identifier_allocator_get_used_mut(&storage->entities, index);
}

extern inline cecs_entity cecs_entity_storage_alloc_entity(cecs_entity_storage *storage, cecs_allocator *allocator);

void cecs_entity_storage_free_entity(cecs_entity_storage *storage, const cecs_entity entity) {
    cecs_entity *const entity_slot = cecs_entity_storage_get_used_mut(storage, cecs_entity_index_of(entity));
    *entity_slot = cecs_entity_next_generation(*entity_slot);
    cecs_identifier_allocator_free(&storage->entities, *entity_slot);
}
