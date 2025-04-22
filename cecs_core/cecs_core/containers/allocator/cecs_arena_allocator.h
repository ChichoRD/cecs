#ifndef CECS_ARENA_ALLOCATOR_H
#define CECS_ARENA_ALLOCATOR_H

#include "cecs_bump_allocator.h"

#ifndef CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE
#define CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_DEFAULT uint8_t

#define CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX UINT8_MAX
#define CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_DEFAULT
#endif

typedef CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE cecs_arena_allocator_bump_usize;


#define CECS_ARENA_ALLOCATOR_BUMP_OWNING_MAX_COUNT (CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX >> 1)
typedef struct cecs_arena_allocator_bump_owning {
    bool owning : 1;
    cecs_arena_allocator_bump_usize owned_count : 7;
} cecs_arena_allocator_bump_owning;
typedef struct cecs_arena_allocator_bump_view {
    bool owning : 1;
    uint8_t unused : 7;
} cecs_arena_allocator_bump_view;
typedef struct cecs_arena_allocator_bump_any {
    bool owning : 1;
    uint8_t uninitialized : 7;
} cecs_arena_allocator_bump_any;
static_assert(
    sizeof(cecs_arena_allocator_bump_owning) == sizeof(cecs_arena_allocator_bump_view)
    && sizeof(cecs_arena_allocator_bump_owning) == sizeof(cecs_arena_allocator_bump_any),
    "fatal error: arena allocator bump status size mismatch"
);

typedef struct cecs_arena_allocator_bump {
    cecs_bump_view_allocator allocator;
    union {
        cecs_arena_allocator_bump_owning owning;
        cecs_arena_allocator_bump_view view;
        cecs_arena_allocator_bump_any any;
    } status;
} cecs_arena_allocator_bump;

typedef struct cecs_arena_allocator {
    cecs_arena_allocator_bump *bumps;
    cecs_arena_allocator_bump_usize current_bump;
    cecs_arena_allocator_bump_usize bump_capacity;
} cecs_arena_allocator;


cecs_arena_allocator cecs_arena_allocator_create(const size_t block_size, const cecs_arena_allocator_bump_usize blocks_capacity);

void *restrict cecs_arena_allocator_alloc_aligned(
    cecs_arena_allocator *allocator, const size_t size, const size_t alignment
);
void *restrict cecs_arena_allocator_alloc(cecs_arena_allocator *allocator, const size_t size);

void *restrict cecs_arena_allocator_realloc_aligned(
    cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
void *restrict cecs_arena_allocator_realloc(
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