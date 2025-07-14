#ifndef CECS_IMPLICIT_ARENA_ALLOCATOR_H
#define CECS_IMPLICIT_ARENA_ALLOCATOR_H

#include "cecs_arena_allocator.h"

#define CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT 6

typedef struct cecs_implicit_arena_allocator_node {
    struct cecs_implicit_arena_allocator_node *next;
    size_t next_size;
} cecs_implicit_arena_allocator_node;

typedef struct cecs_implicit_arena_allocator {
    cecs_arena_allocator arena;
    cecs_implicit_arena_allocator_node smallest_free_block;
    cecs_implicit_arena_allocator_node largest_free_blocks[CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT];
} cecs_implicit_arena_allocator;


inline cecs_implicit_arena_allocator cecs_implicit_arena_allocator_create(const size_t block_size, const cecs_arena_allocator_bump_usize blocks_capacity) {
    return (cecs_implicit_arena_allocator){
        .arena = cecs_arena_allocator_create(block_size, blocks_capacity),
        .largest_free_blocks = {0},
        .smallest_free_block = {0}
    };
}

void *cecs_implicit_arena_allocator_alloc_aligned(
    cecs_implicit_arena_allocator *allocator, const size_t size, const size_t alignment
);
void *cecs_implicit_arena_allocator_alloc(cecs_implicit_arena_allocator *allocator, const size_t size);

void *cecs_implicit_arena_allocator_realloc_aligned(
    cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
void *cecs_implicit_arena_allocator_realloc(
    cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size
);

void cecs_implicit_arena_allocator_free(cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size);
void cecs_implicit_arena_allocator_reset(cecs_implicit_arena_allocator *allocator);

void cecs_implicit_arena_allocator_destroy(cecs_implicit_arena_allocator *allocator);


// inline size_t cecs_implicit_arena_allocator_current_bump_capacity(const cecs_implicit_arena_allocator *allocator);
// inline size_t cecs_implicit_arena_allocator_current_bump_used(const cecs_implicit_arena_allocator *allocator);
// inline ptrdiff_t cecs_implicit_arena_allocator_current_bump_available(const cecs_implicit_arena_allocator *allocator);
// inline ptrdiff_t cecs_implicit_arena_allocator_current_bump_available_aligned(const cecs_implicit_arena_allocator *allocator, const size_t alignment);

#endif