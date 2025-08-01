#include "cecs_arena_allocator.h"

#include <cecs_math/relations/cecs_ordering.h>
#include <stdlib.h>

cecs_arena_allocator cecs_arena_allocator_create(const size_t block_size, const cecs_arena_allocator_bump_usize blocks_capacity) {
    assert(block_size > 0 && "fatal error: block_size must be greater than 0");
    assert(blocks_capacity > 0 && "fatal error: blocks_capacity must be greater than 0");

    cecs_bump_allocator *bumps = cecs_alloc_expect(blocks_capacity * sizeof(cecs_bump_allocator));
    bumps[0] = cecs_bump_allocator_create(block_size);

    if (!CECS_ALLOC_FUNC_IS_ZERO_INIT) {
        for (cecs_arena_allocator_bump_usize i = 1; i < blocks_capacity; ++i) {
            bumps[i] = (cecs_bump_allocator){(cecs_bump_view_allocator){
                .next = NULL,
                .block_start = NULL,
                .block_end = NULL
            }};
        }
    }
    return (cecs_arena_allocator){
        .bumps = bumps,
        .current_bump = 0,
        .bump_capacity = blocks_capacity
    };
}

extern inline cecs_bump_allocator *cecs_arena_allocator_current_bump_mut(cecs_arena_allocator *allocator);
extern inline const cecs_bump_allocator *cecs_arena_allocator_current_bump(const cecs_arena_allocator *allocator);

void *cecs_arena_allocator_alloc_aligned_advance(cecs_arena_allocator *allocator, const size_t size, const size_t alignment) {
    ++allocator->current_bump;
    // TODO: maybe if we pick last block's size we can double it
    const size_t new_blocks_size = cecs_max(cecs_bump_allocator_capacity(&allocator->bumps[allocator->current_bump - 1]), size);
    if (allocator->current_bump == allocator->bump_capacity) {
        const size_t new_blocks_capacity = allocator->bump_capacity << 1;
        assert(
            new_blocks_capacity <= CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX
            && "fatal error: new_blocks_capacity must be less than or equal to CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX"
        );

        cecs_bump_allocator *const new_bumps = cecs_realloc_expect(
            allocator->bumps,
            allocator->bump_capacity * sizeof(cecs_bump_allocator),
            new_blocks_capacity * sizeof(cecs_bump_allocator)
        );
        
        new_bumps[allocator->current_bump] = cecs_bump_allocator_create(new_blocks_size);
        for (cecs_arena_allocator_bump_usize i = allocator->current_bump + 1; i < new_blocks_capacity; ++i) {
            new_bumps[i] = (cecs_bump_allocator){(cecs_bump_view_allocator){
                .next = NULL,
                .block_start = NULL,
                .block_end = NULL
            }};
        }

        allocator->bumps = new_bumps;
        allocator->bump_capacity = new_blocks_capacity;
        return cecs_bump_allocator_alloc_aligned_expect(cecs_arena_allocator_current_bump_mut(allocator), size, alignment);
    } else if (allocator->current_bump < allocator->bump_capacity) {
        cecs_bump_allocator *const current_bump = &allocator->bumps[allocator->current_bump];
        if (current_bump->view.next != NULL) {
            assert(false && "fatal error: allocator's bump must be uninitialized after advancing");
            exit(EXIT_FAILURE);
        }
        *current_bump = cecs_bump_allocator_create(new_blocks_size);
        return cecs_bump_allocator_alloc_aligned_expect(current_bump, size, alignment);
    } else {
        assert(false && "fatal error: allocator's current bump is out of bounds");
        exit(EXIT_FAILURE);
    }
}

void *cecs_arena_allocator_alloc_aligned(cecs_arena_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_bump_allocator *const current_bump = cecs_arena_allocator_current_bump_mut(allocator);
    if (cecs_bump_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)size) {
        return cecs_arena_allocator_alloc_aligned_advance(allocator, size, alignment);
    } else {
        return cecs_bump_allocator_alloc_aligned_expect(current_bump, size, alignment);
    }
}

void *cecs_arena_allocator_alloc(cecs_arena_allocator *allocator, const size_t size)  {
    return cecs_arena_allocator_alloc_aligned(allocator, size, cecs_max_alignment_from_size(size));
}

void *cecs_arena_allocator_realloc_aligned(
    cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    cecs_bump_allocator *const current_bump = cecs_arena_allocator_current_bump_mut(allocator);
    if (cecs_bump_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)new_size) {
        return cecs_arena_allocator_alloc_aligned_advance(allocator, new_size, alignment);
    } else {
        return cecs_bump_allocator_realloc_aligned_expect(current_bump, block, block_size, new_size, alignment);
    }
}

void *cecs_arena_allocator_realloc(cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size) {
    return cecs_arena_allocator_realloc_aligned(allocator, block, block_size, new_size, cecs_max_alignment_from_size(new_size));
}

void cecs_arena_allocator_free(cecs_arena_allocator *allocator, void *block, const size_t block_size) {
    cecs_bump_allocator_free(cecs_arena_allocator_current_bump_mut(allocator), block, block_size);
}

void cecs_arena_allocator_reset(cecs_arena_allocator *allocator) {
    allocator->current_bump = 0;
    for (cecs_arena_allocator_bump_usize i = 0; i < allocator->bump_capacity; ++i) {
        cecs_bump_allocator_reset(&allocator->bumps[i]);
    }
}

void cecs_arena_allocator_destroy(cecs_arena_allocator *allocator) {
    for (cecs_arena_allocator_bump_usize i = 0; i < allocator->bump_capacity; ++i) {
        cecs_bump_allocator_destroy(&allocator->bumps[i]);
    }

    cecs_free_expect(allocator->bumps, allocator->bump_capacity * sizeof(cecs_bump_allocator));
    allocator->bumps = NULL;
    allocator->current_bump = 0;
    allocator->bump_capacity = 0;
}

size_t cecs_arena_allocator_current_bump_capacity(const cecs_arena_allocator *allocator) {
    return cecs_bump_allocator_capacity(cecs_arena_allocator_current_bump(allocator));
}
size_t cecs_arena_allocator_current_bump_used(const cecs_arena_allocator *allocator) {
    return cecs_bump_allocator_used(cecs_arena_allocator_current_bump(allocator));
}
ptrdiff_t cecs_arena_allocator_current_bump_available(const cecs_arena_allocator *allocator) {
    return cecs_bump_allocator_available(cecs_arena_allocator_current_bump(allocator));
}
ptrdiff_t cecs_arena_allocator_current_bump_available_aligned(const cecs_arena_allocator *allocator, const size_t alignment) {
    return cecs_bump_allocator_available_aligned(cecs_arena_allocator_current_bump(allocator), alignment);
}
