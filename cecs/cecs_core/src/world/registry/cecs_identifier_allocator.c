#include "cecs_identifier_allocator.h"
#include <cecs_type_traits.h>
#include <cecs_error.h>
#include <stdbool.h>
#include <stddef.h>

extern inline cecs_identifier_allocator cecs_identifier_allocator_create(void);
extern inline cecs_identifier_allocator cecs_identifier_allocator_create_with_capacity(cecs_allocator *const allocator, const size_t initial_capacity);
extern inline void cecs_identifier_allocator_destroy(cecs_identifier_allocator *const storage, cecs_allocator *const allocator);

extern inline size_t cecs_identifier_allocator_total_count(const cecs_identifier_allocator *const storage);
extern inline size_t cecs_identifier_allocator_free_count(const cecs_identifier_allocator *const storage);
extern inline size_t cecs_identifier_allocator_used_count(const cecs_identifier_allocator *const storage);
extern inline cecs_identifier cecs_identifier_allocator_peek(const cecs_identifier_allocator *const storage, const size_t index);

extern inline cecs_identifier *cecs_identifier_allocator_peek_mut(cecs_identifier_allocator *const storage, const size_t index) {
    cecs_debugbreak_fail_unless(
        index < cecs_identifier_allocator_total_count(storage),
        "error: cecs_identifier_allocator_peek_mut called with out of bounds index"
        );
    return (cecs_identifier*)cecs_dynarray_get_mut(&storage->identifiers, index, sizeof(cecs_identifier));
}
extern inline cecs_identifier *cecs_identifier_allocator_get_used_mut(cecs_identifier_allocator *const storage, const size_t index) {
    cecs_identifier *const identifier = cecs_identifier_allocator_peek_mut(storage, index);
    cecs_debugbreak_fail_unless(
        cecs_identifier_is_used(*identifier, index),
        "error: cecs_identifier_allocator_get_mut called for a free identifier"
    );
    return identifier;
}
static inline cecs_identifier *cecs_identifier_allocator_get_free_mut(cecs_identifier_allocator *const storage, const size_t index) {
    cecs_identifier *const identifier = cecs_identifier_allocator_peek_mut(storage, index);
    cecs_debugbreak_fail_unless(
        cecs_identifier_is_free(*identifier, index),
        "error: cecs_identifier_allocator_get_free_mut called for a used identifier"
    );
    return identifier;
}


cecs_identifier cecs_identifier_allocator_get_used(const cecs_identifier_allocator *const storage, const size_t index) {
    const cecs_identifier identifier = cecs_identifier_allocator_peek(storage, index);
    cecs_debugbreak_fail_unless(
        cecs_identifier_is_used(identifier, index),
        "error: cecs_identifier_allocator_get_used called for a free identifier\n"
        "note: use cecs_identifier_allocator_peek to get an identifier when it is not certain whether it is free or used"
    );
    return identifier;
}
cecs_identifier cecs_identifier_allocator_get_free(const cecs_identifier_allocator *const storage, const size_t index) {
    const cecs_identifier identifier = cecs_identifier_allocator_peek(storage, index);
    cecs_debugbreak_fail_unless(
        cecs_identifier_is_free(identifier, index),
        "error: cecs_identifier_allocator_get_free called for a used identifier\n"
        "note: use cecs_identifier_allocator_peek to get an identifier when it is not certain whether it is free or used"
    );
    return identifier;
}
cecs_identifier cecs_identifier_allocator_get_exact(const cecs_identifier_allocator *const storage, const cecs_identifier identifier) {
    const size_t index = cecs_identifier_index_of(identifier);
    const cecs_identifier stored_identifier = cecs_identifier_allocator_peek(storage, index);
    cecs_debugbreak_fail_unless(
        stored_identifier.value == identifier.value,
        "error: cecs_identifier_allocator_get_exact called with mismatched identifier"
    );
    return stored_identifier;
}

cecs_identifier cecs_identifier_allocator_alloc(cecs_identifier_allocator *const storage, cecs_allocator *const allocator) {
    if (storage->free_count == 0) {
        const size_t index = cecs_identifier_allocator_total_count(storage);
        cecs_identifier *const identifier = cecs_dynarray_push(&storage->identifiers, allocator, sizeof(cecs_identifier));
        *identifier = cecs_identifier_create(index, 0ull);
        return *identifier;
    } else {
        const size_t index = cecs_identifier_index_of(storage->next_free);
        cecs_identifier *const identifier = cecs_identifier_allocator_get_free_mut(storage, index);

        const cecs_identifier next_free = storage->next_free;
        storage->next_free = *identifier;
        *identifier = next_free;
        --storage->free_count;
        return *identifier;
    }
}
void cecs_identifier_allocator_free(cecs_identifier_allocator *const storage, const cecs_identifier identifier) {
    cecs_identifier *const stored_identifier = cecs_identifier_allocator_get_used_mut(storage, cecs_identifier_index_of(identifier));
    const cecs_identifier next_free = storage->next_free;
    storage->next_free = *stored_identifier;
    *stored_identifier = next_free;
    ++storage->free_count;
}
