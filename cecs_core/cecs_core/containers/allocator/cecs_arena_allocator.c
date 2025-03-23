#include "cecs_arena_allocator.h"

#include <cecs_math/relations/cecs_ordering.h>

cecs_arena_allocator cecs_arena_allocator_create(const size_t block_size, const cecs_arena_allocator_bump_index blocks_capacity) {
    assert(block_size > 0 && "fatal error: block_size must be greater than 0");
    assert(blocks_capacity > 0 && "fatal error: blocks_capacity must be greater than 0");

    cecs_bump_allocator *bumps = cecs_alloc_expect(blocks_capacity * sizeof(cecs_bump_allocator));
    uint8_t *const arena = cecs_alloc_expect(blocks_capacity * block_size);

    uint8_t *bump = arena;
    for (cecs_arena_allocator_bump_index i = 0; i < blocks_capacity; ++i) {
        uint8_t *const next_bump = bump + block_size;
        bumps[i] = cecs_bump_allocator_create_from(bump, next_bump);
        bump = next_bump;
    }

    return (cecs_arena_allocator){
        .bumps = bumps,
        .current_bump = 0,
        .bump_capacity = blocks_capacity
    };
}

static inline cecs_bump_allocator *cecs_arena_allocator_current_bump(cecs_arena_allocator *allocator) {
    assert(allocator->current_bump < allocator->bump_capacity && "fatal error: allocator's current bump is out of bounds");
    return &allocator->bumps[allocator->current_bump];
}

static void *restrict cecs_arena_allocator_alloc_aligned_advance(cecs_arena_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_bump_allocator *const current_bump = cecs_arena_allocator_current_bump(allocator);
    ++allocator->current_bump;
    if (allocator->current_bump == allocator->bump_capacity) {
        const size_t new_blocks_size = cecs_max(cecs_bump_allocator_capacity(current_bump), size);
        const size_t new_blocks_capacity = allocator->bump_capacity << 1;

        cecs_bump_allocator *const new_bumps = cecs_realloc_expect(
            allocator->bumps, allocator->bump_capacity * sizeof(cecs_bump_allocator), new_blocks_capacity * sizeof(cecs_bump_allocator)
        );

        const size_t new_arena_size = allocator->bump_capacity * new_blocks_size;
        uint8_t *const new_arena = cecs_alloc_expect(new_arena_size);

        uint8_t *new_bump = new_arena;
        for (cecs_arena_allocator_bump_index i = allocator->current_bump; i < new_blocks_capacity; ++i) {
            uint8_t *const next_bump = new_bump + new_blocks_size;
            new_bumps[i] = cecs_bump_allocator_create_from(new_bump, next_bump);
            new_bump = next_bump;
        }

        allocator->bumps = new_bumps;
        allocator->bump_capacity = new_blocks_capacity;
        return cecs_bump_allocator_alloc_aligned_expect(cecs_arena_allocator_current_bump(allocator), size, alignment);
    } else if (allocator->current_bump < allocator->bump_capacity) {
        return cecs_bump_allocator_alloc_aligned_expect(cecs_arena_allocator_current_bump(allocator), size, alignment);
    } else {
        assert(false && "fatal error: allocator's current bump is out of bounds");
    }
}

void *restrict cecs_arena_allocator_alloc_aligned(cecs_arena_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_bump_allocator *const current_bump = cecs_arena_allocator_current_bump(allocator);
    if (cecs_bump_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)size) {
        return cecs_arena_allocator_alloc_aligned_advance(allocator, size, alignment);
    } else {
        return cecs_bump_allocator_alloc_aligned_expect(current_bump, size, alignment);
    }
}

void *restrict cecs_arena_allocator_alloc(cecs_arena_allocator *allocator, const size_t size)  {
    return cecs_arena_allocator_alloc_aligned(allocator, size, cecs_max_alignment_from_size(size));
}

void *restrict cecs_arena_allocator_realloc_aligned(
    cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    cecs_bump_allocator *const current_bump = cecs_arena_allocator_current_bump(allocator);
    if (cecs_bump_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)new_size) {
        return cecs_arena_allocator_alloc_aligned_advance(allocator, new_size, alignment);
    } else {
        return cecs_bump_allocator_realloc_aligned_expect(current_bump, block, block_size, new_size, alignment);
    }
}

void *restrict cecs_arena_allocator_realloc(cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size) {
    return cecs_arena_allocator_realloc_aligned(allocator, block, block_size, new_size, cecs_max_alignment_from_size(new_size));
}

void cecs_arena_allocator_free(cecs_arena_allocator *allocator, void *block, const size_t block_size) {
    cecs_bump_allocator_free(cecs_arena_allocator_current_bump(allocator), block, block_size);
}

void cecs_arena_allocator_reset(cecs_arena_allocator *allocator) {
    allocator->current_bump = 0;
    for (cecs_arena_allocator_bump_index i = 0; i < allocator->bump_capacity; ++i) {
        cecs_bump_allocator_reset(&allocator->bumps[i]);
    }
}

void cecs_arena_allocator_destroy(cecs_arena_allocator *allocator) {
    cecs_bump_allocator *previous_bump = allocator->bumps;
    cecs_bump_allocator_destroy(previous_bump);

    for (cecs_arena_allocator_bump_index i = 1; i < allocator->bump_capacity; ++i) {
        cecs_bump_allocator *const bump = &allocator->bumps[i];
        if (bump->block_start != previous_bump->block_end + 1) {
            cecs_bump_allocator_destroy(bump);
        }
        previous_bump = bump;
    }

    cecs_free_expect(allocator->bumps, allocator->bump_capacity * sizeof(cecs_bump_allocator));
    allocator->bumps = NULL;
    allocator->current_bump = 0;
    allocator->bump_capacity = 0;
}
