#ifndef CECS_ARENA_ALLOCATOR_H
#define CECS_ARENA_ALLOCATOR_H

#include "cecs_bump_allocator.h"
#include <stdint.h>

#ifndef CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE
#define CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_DEFAULT uint32_t

#define CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX UINT32_MAX
#define CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_DEFAULT
#endif

typedef CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE cecs_arena_allocator_bump_usize;

typedef struct cecs_arena_allocator {
    cecs_bump_allocator *bumps;
    cecs_arena_allocator_bump_usize current_bump;
    cecs_arena_allocator_bump_usize bump_capacity;
} cecs_arena_allocator;


cecs_arena_allocator cecs_arena_allocator_create(const size_t block_size, const cecs_arena_allocator_bump_usize blocks_capacity);

void *cecs_arena_allocator_alloc_aligned(
    cecs_arena_allocator *allocator, const size_t size, const size_t alignment
);
void *cecs_arena_allocator_alloc(cecs_arena_allocator *allocator, const size_t size);

void *cecs_arena_allocator_realloc_aligned(
    cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
void *cecs_arena_allocator_realloc(
    cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size
);

void cecs_arena_allocator_free(cecs_arena_allocator *allocator, void *block, const size_t block_size);
void cecs_arena_allocator_reset(cecs_arena_allocator *allocator);

void cecs_arena_allocator_destroy(cecs_arena_allocator *allocator);


size_t cecs_arena_allocator_current_bump_capacity(const cecs_arena_allocator *allocator);
size_t cecs_arena_allocator_current_bump_used(const cecs_arena_allocator *allocator);
ptrdiff_t cecs_arena_allocator_current_bump_available(const cecs_arena_allocator *allocator);
ptrdiff_t cecs_arena_allocator_current_bump_available_aligned(const cecs_arena_allocator *allocator, const size_t alignment);

#endif