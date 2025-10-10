#ifndef CECS_ALLOCATOR_H
#define CECS_ALLOCATOR_H

#include "allocator/cecs_bump_allocator.h"
#include "allocator/cecs_arena_allocator.h"
#include "allocator/cecs_implicit_arena_allocator.h"

typedef union cecs_internal_allocator {
    cecs_bump_allocator bump;
    cecs_arena_allocator arena;
    cecs_implicit_arena_allocator implicit_arena;
} cecs_internal_allocator;
typedef enum cecs_internal_allocator_type {
    cecs_internal_allocator_bump,
    cecs_internal_allocator_arena,
    cecs_internal_allocator_implicit_arena,
} cecs_internal_allocator_type;

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
    if (allocator->type != cecs_internal_allocator_bump) {
        assert(false && "fatal error: tried to access wrong variant (bump) of allocator");
        exit(EXIT_FAILURE);
    }
    return &allocator->allocator.bump;
}
inline cecs_bump_allocator *cecs_allocator_bump_mut(cecs_allocator *allocator) {
    if (allocator->type != cecs_internal_allocator_bump) {
        assert(false && "fatal error: tried to access wrong variant (bump) of allocator");
        exit(EXIT_FAILURE);
    }
    return &allocator->allocator.bump;
}
inline const cecs_arena_allocator *cecs_allocator_arena(const cecs_allocator *allocator) {
    if (allocator->type != cecs_internal_allocator_arena) {
        assert(false && "fatal error: tried to access wrong variant (arena) of allocator");
        exit(EXIT_FAILURE);
    }
    return &allocator->allocator.arena;
}
inline cecs_arena_allocator *cecs_allocator_arena_mut(cecs_allocator *allocator) {
    if (allocator->type != cecs_internal_allocator_arena) {
        assert(false && "fatal error: tried to access wrong variant (arena) of allocator");
        exit(EXIT_FAILURE);
    }
    return &allocator->allocator.arena;
}
inline const cecs_implicit_arena_allocator *cecs_allocator_implicit_arena(const cecs_allocator *allocator) {
    if (allocator->type != cecs_internal_allocator_implicit_arena) {
        assert(false && "fatal error: tried to access wrong variant (implicit arena) of allocator");
        exit(EXIT_FAILURE);
    }
    return &allocator->allocator.implicit_arena;
}
inline cecs_implicit_arena_allocator *cecs_allocator_implicit_arena_mut(cecs_allocator *allocator) {
    if (allocator->type != cecs_internal_allocator_implicit_arena) {
        assert(false && "fatal error: tried to access wrong variant (implicit arena) of allocator");
        exit(EXIT_FAILURE);
    }
    return &allocator->allocator.implicit_arena;
}

#endif
