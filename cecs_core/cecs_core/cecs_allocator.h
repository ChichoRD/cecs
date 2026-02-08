#ifndef CECS_ALLOCATOR_H
#define CECS_ALLOCATOR_H

#include "allocator/cecs_bump_allocator.h"
#include "allocator/cecs_arena_allocator.h"
#include "allocator/cecs_implicit_arena_allocator.h"
#include <cecs_core/cecs_error.h>

typedef union cecs_internal_allocator {
    cecs_bump_allocator bump;
    cecs_arena_allocator arena;
    cecs_implicit_arena_allocator implicit_arena;
} cecs_internal_allocator;
typedef enum cecs_internal_allocator_type {
    cecs_internal_allocator_type_none = 0,
    cecs_internal_allocator_type_bump,
    cecs_internal_allocator_type_arena,
    cecs_internal_allocator_type_implicit_arena,
} cecs_internal_allocator_type;

// TODO: establish policy for NULL free
// TODO: establish policy for zero-sized allocations
typedef struct cecs_allocator {
    cecs_internal_allocator allocator;
    cecs_internal_allocator_type type;
} cecs_allocator;

cecs_allocator cecs_allocator_create_bump_alloc(const size_t size);
cecs_allocator cecs_allocator_create_bump_virtual(const size_t page_count);
cecs_allocator cecs_allocator_create_arena(const size_t bump_size, const size_t bump_capacity);
cecs_allocator cecs_allocator_create_implicit_arena(const size_t bump_size, const size_t bump_capacity);

void *cecs_allocator_alloc_aligned(cecs_allocator *allocator, const size_t size, const size_t alignment);
void *cecs_allocator_alloc(cecs_allocator *allocator, const size_t size);

void *cecs_allocator_realloc_aligned(cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment);
void *cecs_allocator_realloc(cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size);

void cecs_allocator_free(cecs_allocator *allocator, void *block, const size_t block_size);
void cecs_allocator_reset(cecs_allocator *allocator);

void cecs_allocator_destroy(cecs_allocator *allocator);

inline const cecs_bump_allocator *cecs_allocator_bump(const cecs_allocator *allocator) {
    cecs_assert_or_exit(allocator->type == cecs_internal_allocator_type_bump, "fatal error: tried to access wrong variant (bump) of allocator");
    return &allocator->allocator.bump;
}
inline cecs_bump_allocator *cecs_allocator_bump_mut(cecs_allocator *allocator) {
    cecs_assert_or_exit(allocator->type == cecs_internal_allocator_type_bump, "fatal error: tried to access wrong variant (bump) of allocator");
    return &allocator->allocator.bump;
}
inline const cecs_arena_allocator *cecs_allocator_arena(const cecs_allocator *allocator) {
    cecs_assert_or_exit(allocator->type == cecs_internal_allocator_type_arena, "fatal error: tried to access wrong variant (arena) of allocator");
    return &allocator->allocator.arena;
}
inline cecs_arena_allocator *cecs_allocator_arena_mut(cecs_allocator *allocator) {
    cecs_assert_or_exit(allocator->type == cecs_internal_allocator_type_arena, "fatal error: tried to access wrong variant (arena) of allocator");
    return &allocator->allocator.arena;
}
inline const cecs_implicit_arena_allocator *cecs_allocator_implicit_arena(const cecs_allocator *allocator) {
    cecs_assert_or_exit(allocator->type == cecs_internal_allocator_type_implicit_arena, "fatal error: tried to access wrong variant (implicit arena) of allocator");
    return &allocator->allocator.implicit_arena;
}
inline cecs_implicit_arena_allocator *cecs_allocator_implicit_arena_mut(cecs_allocator *allocator) {
    cecs_assert_or_exit(allocator->type == cecs_internal_allocator_type_implicit_arena, "fatal error: tried to access wrong variant (implicit arena) of allocator");
    return &allocator->allocator.implicit_arena;
}

cecs_allocator cecs_allocator_alloc_bump_view(cecs_allocator *allocator, const size_t size);
cecs_allocator cecs_allocator_alloc_bump_view_aligned(cecs_allocator *allocator, const size_t size, const size_t alignment);


#endif
