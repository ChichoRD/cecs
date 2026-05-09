#ifndef CECS_IDENTIFIER_ALLOCATOR_H
#define CECS_IDENTIFIER_ALLOCATOR_H

#include "cecs_identifier.h"
#include <cecs_dynarray.h>

// base on source: https://skypjack.github.io/2019-05-06-ecs-baf-part-3/
typedef struct cecs_identifier_allocator {
    cecs_dynarray identifiers;
    cecs_identifier next_free;
    size_t free_count;
} cecs_identifier_allocator;

inline cecs_identifier_allocator cecs_identifier_allocator_create(void) {
    return (cecs_identifier_allocator){
        .identifiers = cecs_dynarray_create(),
        .next_free = cecs_identifier_create(CECS_IDENTIFIER_INDEX_MAX, 0ull),
        .free_count = 0ull,
    };
}
inline cecs_identifier_allocator cecs_identifier_allocator_create_with_capacity(cecs_allocator *const allocator, const size_t initial_capacity) {
    return (cecs_identifier_allocator){
        .identifiers = cecs_dynarray_create_with_capacity(allocator, initial_capacity, sizeof(cecs_identifier)),
        .next_free = cecs_identifier_create(CECS_IDENTIFIER_INDEX_MAX, 0ull),
        .free_count = 0ull,
    };
}
static inline void cecs_identifier_allocator_reset(cecs_identifier_allocator *const storage) {
    storage->free_count = 0ull;
    storage->next_free = cecs_identifier_create(CECS_IDENTIFIER_INDEX_MAX, 0ull);
    cecs_dynarray_clear(&storage->identifiers);
}
inline void cecs_identifier_allocator_destroy(cecs_identifier_allocator *const storage, cecs_allocator *const allocator) {
    storage->free_count = 0ull;
    storage->next_free = cecs_identifier_create(CECS_IDENTIFIER_INDEX_MAX, 0ull);
    cecs_dynarray_destroy(&storage->identifiers, allocator, sizeof(cecs_identifier));
}

inline size_t cecs_identifier_allocator_total_count(const cecs_identifier_allocator *const storage) {
    return cecs_dynarray_count(&storage->identifiers);
}
inline size_t cecs_identifier_allocator_free_count(const cecs_identifier_allocator *const storage) {
    return storage->free_count;
}
inline size_t cecs_identifier_allocator_used_count(const cecs_identifier_allocator *const storage) {
    return cecs_identifier_allocator_total_count(storage) - storage->free_count;
}

inline cecs_identifier cecs_identifier_allocator_peek(const cecs_identifier_allocator *const storage, const size_t index) {
    cecs_debugbreak_fail_unless(
        index < cecs_identifier_allocator_total_count(storage),
        "error: cecs_identifier_allocator_peek called with out of bounds index"
    );
    return *((const cecs_identifier*)cecs_dynarray_get(&storage->identifiers, index, sizeof(cecs_identifier)));
}
cecs_identifier cecs_identifier_allocator_get_used(const cecs_identifier_allocator *const storage, const size_t index);
cecs_identifier cecs_identifier_allocator_get_free(const cecs_identifier_allocator *const storage, const size_t index);
cecs_identifier cecs_identifier_allocator_get_exact(const cecs_identifier_allocator *const storage, const cecs_identifier identifier);

// TODO: think about policy about returned meta bits of the identifier
// XXX: now they are either 0 or the meta bits of the last freed identifier for that index
cecs_identifier cecs_identifier_allocator_alloc(cecs_identifier_allocator *const storage, cecs_allocator *const allocator);
void cecs_identifier_allocator_free(cecs_identifier_allocator *const storage, const cecs_identifier identifier);


#endif
