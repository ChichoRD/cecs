#ifndef CECS_POOL_ALLOCATOR_H
#define CECS_POOL_ALLOCATOR_H

#include "cecs_arena_allocator.h"

typedef cecs_arena_allocator_bump_usize cecs_pool_allocator_block_usize;

typedef struct cecs_pool_allocator_free_block {
    uint8_t *restrict block;
    size_t size;
    cecs_pool_allocator_block_usize next_largest_block_index;
    cecs_pool_allocator_block_usize ref_index;
} cecs_pool_allocator_free_block;
typedef struct cecs_pool_allocator_free_block_ref {
    uint32_t max_size_log2 : 6;
    uint32_t index : 26;
} cecs_pool_allocator_free_block_ref;
static_assert(
    sizeof(cecs_pool_allocator_free_block_ref) == sizeof(uint32_t),
    "fatal error: cecs_pool_allocator_free_block_ref must be 32 bits"
);

typedef struct cecs_pool_allocator {
    cecs_arena_allocator arena;
    cecs_pool_allocator_free_block *free_blocks;
    cecs_pool_allocator_free_block_ref *free_block_refs;
    size_t max_free_block_size;
    cecs_pool_allocator_block_usize free_blocks_count;
} cecs_pool_allocator;

cecs_pool_allocator cecs_pool_allocator_create(const size_t block_size, const cecs_pool_allocator_block_usize blocks_capacity);

void *restrict cecs_pool_allocator_alloc_aligned(cecs_pool_allocator *allocator, const size_t size, const size_t alignment);
void *restrict cecs_pool_allocator_alloc(cecs_pool_allocator *allocator, const size_t size);

void *restrict cecs_pool_allocator_realloc_aligned(
    cecs_pool_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
void *restrict cecs_pool_allocator_realloc(
    cecs_pool_allocator *allocator, void *block, const size_t block_size, const size_t new_size
);

void cecs_pool_allocator_free(cecs_pool_allocator *allocator, void *block, const size_t size);
void cecs_pool_allocator_reset(cecs_pool_allocator *allocator);

void cecs_pool_allocator_destroy(cecs_pool_allocator *allocator);

#endif