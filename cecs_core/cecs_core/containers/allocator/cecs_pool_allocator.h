#ifndef CECS_POOL_ALLOCATOR_H
#define CECS_POOL_ALLOCATOR_H

#include "cecs_arena_allocator.h"

#ifndef CECS_POOL_ALLOCATOR_GRAIN_SIZE_TYPE
#define CECS_POOL_ALLOCATOR_GRAIN_SIZE_TYPE_DEFAULT uint8_t

#define CECS_POOL_ALLOCATOR_GRAIN_SIZE_TYPE CECS_POOL_ALLOCATOR_GRAIN_SIZE_TYPE_DEFAULT
#endif


#ifndef CECS_POOL_ALLOCATOR_MASK_TYPE
#define CECS_POOL_ALLOCATOR_MASK_TYPE_DEFAULT size_t

#undef CECS_POOL_ALLOCATOR_MASK_TYPE_MAX
#define CECS_POOL_ALLOCATOR_MASK_TYPE_MAX SIZE_MAX

#define CECS_POOL_ALLOCATOR_MASK_TYPE CECS_POOL_ALLOCATOR_MASK_TYPE_DEFAULT
#endif

#if !defined(CECS_POOL_ALLOCATOR_MASK_TYPE_MAX)
static_assert(
    false,
    "static error: CECS_POOL_ALLOCATOR_MASK_TYPE_MAX must be defined"
)
#endif

typedef CECS_POOL_ALLOCATOR_GRAIN_SIZE_TYPE cecs_pool_allocator_grain_size;
typedef CECS_POOL_ALLOCATOR_MASK_TYPE cecs_pool_allocator_mask;
typedef struct cecs_pool_allocator {
    cecs_arena_allocator arena;
    cecs_pool_allocator_mask *free_mask;
    size_t first_free_mask_index;
    cecs_pool_allocator_grain_size granularity_log2;
} cecs_pool_allocator;

cecs_pool_allocator cecs_pool_allocator_create_u64(const size_t block_size, const cecs_arena_allocator_bump_index blocks_capacity);
cecs_pool_allocator cecs_pool_allocator_create_u128(const size_t block_size, const cecs_arena_allocator_bump_index blocks_capacity);
cecs_pool_allocator cecs_pool_allocator_create_u256(const size_t block_size, const cecs_arena_allocator_bump_index blocks_capacity);

void *restrict cecs_pool_allocator_alloc_aligned(cecs_pool_allocator *allocator, const size_t size, const size_t alignment);
void *restrict cecs_pool_allocator_alloc(cecs_pool_allocator *allocator, const size_t size);

void *restrict cecs_pool_allocator_realloc_aligned(
    cecs_pool_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
void *restrict cecs_pool_allocator_realloc(
    cecs_pool_allocator *allocator, void *block, const size_t block_size, const size_t new_size
);

void cecs_pool_allocator_free(cecs_pool_allocator *allocator, void *block, const size_t block_size);
void cecs_pool_allocator_reset(cecs_pool_allocator *allocator);

void cecs_pool_allocator_destroy(cecs_pool_allocator *allocator);

#endif