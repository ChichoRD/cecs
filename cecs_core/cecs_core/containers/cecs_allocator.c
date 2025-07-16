#include "cecs_allocator.h"

inline cecs_allocator cecs_allocator_create_from(
    const cecs_internal_allocator allocator,
    const cecs_internal_allocator_type type
) {
    return (cecs_allocator){
        .allocator = allocator,
        .type = type
    };
}

cecs_allocator cecs_allocator_create_bump(const size_t size) {
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .bump = cecs_bump_allocator_create(size) },
        cecs_internal_allocator_bump
    );
}
cecs_allocator cecs_allocator_create_arena(size_t bump_size, size_t bump_capacity) {
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .arena = cecs_arena_allocator_create(bump_size, bump_capacity) },
        cecs_internal_allocator_arena
    );
}
cecs_allocator cecs_allocator_create_implicit_arena(size_t bump_size, size_t bump_capacity) {
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .implicit_arena = cecs_implicit_arena_allocator_create(bump_size, bump_capacity) },
        cecs_internal_allocator_implicit_arena
    );
}

void *cecs_allocator_alloc_aligned(cecs_allocator *allocator, const size_t size, const size_t alignment)
{
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        return cecs_bump_allocator_alloc_aligned_expect(&allocator->allocator.bump, size, alignment);
    case cecs_internal_allocator_arena:
        return cecs_arena_allocator_alloc_aligned_expect(&allocator->allocator.arena, size, alignment);
    case cecs_internal_allocator_implicit_arena:
        return cecs_implicit_arena_allocator_alloc_aligned(&allocator->allocator.implicit_arena, size, alignment);
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void *cecs_allocator_alloc(cecs_allocator *allocator, const size_t size) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        return cecs_bump_allocator_alloc_expect(&allocator->allocator.bump, size);
    case cecs_internal_allocator_arena:
        return cecs_arena_allocator_alloc_expect(&allocator->allocator.arena, size);
    case cecs_internal_allocator_implicit_arena:
        return cecs_implicit_arena_allocator_alloc(&allocator->allocator.implicit_arena, size);
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void *cecs_allocator_realloc_aligned(
    cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        return cecs_bump_allocator_realloc_aligned_expect(&allocator->allocator.bump, block, block_size, new_size, alignment);
    case cecs_internal_allocator_arena:
        return cecs_arena_allocator_realloc_aligned_expect(&allocator->allocator.arena, block, block_size, new_size, alignment);
    case cecs_internal_allocator_implicit_arena:
        return cecs_implicit_arena_allocator_realloc_aligned(
            &allocator->allocator.implicit_arena, block, block_size, new_size, alignment
        );
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void *cecs_allocator_realloc(cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        return cecs_bump_allocator_realloc_expect(&allocator->allocator.bump, block, block_size, new_size);
    case cecs_internal_allocator_arena:
        return cecs_arena_allocator_realloc_expect(&allocator->allocator.arena, block, block_size, new_size);
    case cecs_internal_allocator_implicit_arena:
        return cecs_implicit_arena_allocator_realloc(
            &allocator->allocator.implicit_arena, block, block_size, new_size
        );
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void cecs_allocator_free(cecs_allocator *allocator, void *block, const size_t block_size) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        cecs_bump_allocator_free(&allocator->allocator.bump, block, block_size);
        break;
    case cecs_internal_allocator_arena:
        cecs_arena_allocator_free(&allocator->allocator.arena, block, block_size);
        break;
    case cecs_internal_allocator_implicit_arena:
        cecs_implicit_arena_allocator_free(&allocator->allocator.implicit_arena, block, block_size);
        break;
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void cecs_allocator_reset(cecs_allocator *allocator) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        cecs_allocator_reset(&allocator->allocator.bump);
        break;
    case cecs_internal_allocator_arena:
        cecs_arena_allocator_reset(&allocator->allocator.arena);
        break;
    case cecs_internal_allocator_implicit_arena:
        cecs_implicit_arena_allocator_reset(&allocator->allocator.implicit_arena);
        break;
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void cecs_allocator_destroy(cecs_allocator *allocator) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        cecs_bump_allocator_destroy(&allocator->allocator.bump);
        break;
    case cecs_internal_allocator_arena:
        cecs_arena_allocator_destroy(&allocator->allocator.arena);
        break;
    case cecs_internal_allocator_implicit_arena:
        cecs_implicit_arena_allocator_destroy(&allocator->allocator.implicit_arena);
        break;
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}
