#include "cecs_arena_allocator.h"

#include <cecs_math/relations/cecs_ordering.h>

cecs_arena_allocator cecs_arena_allocator_create(const size_t block_size, const cecs_arena_allocator_bump_index blocks_capacity) {
    assert(block_size > 0 && "fatal error: block_size must be greater than 0");
    assert(blocks_capacity > 0 && "fatal error: blocks_capacity must be greater than 0");
    assert(
        blocks_capacity <= CECS_ARENA_ALLOCATOR_BUMP_INDEX_TYPE_MAX
        && "fatal error: blocks_capacity must be less than or equal to CECS_ARENA_ALLOCATOR_BUMP_INDEX_TYPE_MAX"
    );

    cecs_arena_allocator_bump *bumps = cecs_alloc_expect(blocks_capacity * sizeof(cecs_arena_allocator_bump));
    uint8_t *const arena = cecs_alloc_expect(blocks_capacity * block_size);

    uint8_t *bump = arena;
    uint8_t *const bump_end = bump + block_size;
    bumps[0] = (cecs_arena_allocator_bump){
        .allocator = cecs_bump_view_allocator_create(bump, bump_end),
        .status.owning = (cecs_arena_allocator_bump_owning){
            .owning = true,
            .owned_count = blocks_capacity
        }
    };

    bump = bump_end;
    for (size_t i = 1; i < blocks_capacity; i++) {
        uint8_t *const next_bump = bump + block_size;
        bumps[i] = (cecs_arena_allocator_bump){
            .allocator = cecs_bump_view_allocator_create(bump, next_bump),
            .status.view = (cecs_arena_allocator_bump_view){
                .owning = false
            }
        };
        bump = next_bump;
    }

    return (cecs_arena_allocator){
        .bumps = bumps,
        .current_bump = 0,
        .bump_capacity = blocks_capacity
    };
}

static inline cecs_bump_view_allocator *cecs_arena_allocator_current_bump(cecs_arena_allocator *allocator) {
    assert(allocator->current_bump < allocator->bump_capacity && "fatal error: allocator's current bump is out of bounds");
    cecs_bump_view_allocator *const current = &allocator->bumps[allocator->current_bump].allocator;
    assert(current->next != NULL && "fatal error: allocator's current bump is empty");
    return current;
}

static void *restrict cecs_arena_allocator_alloc_aligned_advance(cecs_arena_allocator *allocator, const size_t size, const size_t alignment) {
    ++allocator->current_bump;
    if (allocator->current_bump == allocator->bump_capacity) {
        cecs_bump_view_allocator *const current_bump = cecs_arena_allocator_current_bump(allocator);
        const size_t new_blocks_size = cecs_max(cecs_bump_view_allocator_capacity(current_bump), size);
        const size_t new_blocks_capacity = allocator->bump_capacity << 1;

        cecs_arena_allocator_bump *const new_bumps = cecs_realloc_expect(
            allocator->bumps,
            allocator->bump_capacity * sizeof(cecs_arena_allocator_bump),
            new_blocks_capacity * sizeof(cecs_arena_allocator_bump)
        );

        const size_t new_arena_size = allocator->bump_capacity * new_blocks_size;
        uint8_t *const new_arena = cecs_alloc_expect(new_arena_size);

        uint8_t *new_bump = new_arena;
        uint8_t *const new_bump_end = new_bump + new_blocks_size;
        new_bumps[allocator->current_bump] = (cecs_arena_allocator_bump){
            .allocator = cecs_bump_view_allocator_create(new_bump, new_bump_end),
            .status.owning = (cecs_arena_allocator_bump_owning){
                .owning = true,
                .owned_count = new_blocks_capacity
            }
        };

        new_bump = new_bump_end;
        for (cecs_arena_allocator_bump_index i = allocator->current_bump + 1; i < new_blocks_capacity; ++i) {
            uint8_t *const next_bump = new_bump + new_blocks_size;
            new_bumps[i] = (cecs_arena_allocator_bump){
                .allocator = cecs_bump_view_allocator_create(new_bump, next_bump),
                .status.view = (cecs_arena_allocator_bump_view){
                    .owning = false
                }
            };
            new_bump = next_bump;
        }

        allocator->bumps = new_bumps;
        allocator->bump_capacity = new_blocks_capacity;
        return cecs_bump_view_allocator_alloc_aligned_expect(cecs_arena_allocator_current_bump(allocator), size, alignment);
    } else if (allocator->current_bump < allocator->bump_capacity) {
        return cecs_bump_view_allocator_alloc_aligned_expect(cecs_arena_allocator_current_bump(allocator), size, alignment);
    } else {
        assert(false && "fatal error: allocator's current bump is out of bounds");
    }
}

void *restrict cecs_arena_allocator_alloc_aligned(cecs_arena_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_bump_view_allocator *const current_bump = cecs_arena_allocator_current_bump(allocator);
    if (cecs_bump_view_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)size) {
        return cecs_arena_allocator_alloc_aligned_advance(allocator, size, alignment);
    } else {
        return cecs_bump_view_allocator_alloc_aligned_expect(current_bump, size, alignment);
    }
}

void *restrict cecs_arena_allocator_alloc(cecs_arena_allocator *allocator, const size_t size)  {
    return cecs_arena_allocator_alloc_aligned(allocator, size, cecs_max_alignment_from_size(size));
}

void *restrict cecs_arena_allocator_realloc_aligned(
    cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    cecs_bump_view_allocator *const current_bump = cecs_arena_allocator_current_bump(allocator);
    if (cecs_bump_view_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)new_size) {
        return cecs_arena_allocator_alloc_aligned_advance(allocator, new_size, alignment);
    } else {
        return cecs_bump_view_allocator_realloc_aligned_expect(current_bump, block, block_size, new_size, alignment);
    }
}

void *restrict cecs_arena_allocator_realloc(cecs_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size) {
    return cecs_arena_allocator_realloc_aligned(allocator, block, block_size, new_size, cecs_max_alignment_from_size(new_size));
}

void cecs_arena_allocator_free(cecs_arena_allocator *allocator, void *block, const size_t block_size) {
    cecs_bump_view_allocator_free(cecs_arena_allocator_current_bump(allocator), block, block_size);
}

void cecs_arena_allocator_reset(cecs_arena_allocator *allocator) {
    allocator->current_bump = 0;
    for (cecs_arena_allocator_bump_index i = 0; i < allocator->bump_capacity; ++i) {
        cecs_bump_view_allocator_reset(&allocator->bumps[i].allocator);
    }
}

void cecs_arena_allocator_destroy(cecs_arena_allocator *allocator) {
    for (cecs_arena_allocator_bump_index i = 0; i < allocator->bump_capacity; ++i) {
        cecs_arena_allocator_bump *const bump = &allocator->bumps[i];
        if (bump->status.any.owning) {
            cecs_bump_view_allocator_destroy(&bump->allocator);
        }
    }

    cecs_free_expect(allocator->bumps, allocator->bump_capacity * sizeof(cecs_bump_view_allocator));
    allocator->bumps = NULL;
    allocator->current_bump = 0;
    allocator->bump_capacity = 0;
}

size_t cecs_arena_allocator_current_bump_capacity(const cecs_arena_allocator *allocator) {
    return cecs_bump_view_allocator_capacity(cecs_arena_allocator_current_bump(allocator));
}
size_t cecs_arena_allocator_current_bump_used(const cecs_arena_allocator *allocator) {
    return cecs_bump_view_allocator_used(cecs_arena_allocator_current_bump(allocator));
}
ptrdiff_t cecs_arena_allocator_current_bump_available(const cecs_arena_allocator *allocator) {
    return cecs_bump_view_allocator_available(cecs_arena_allocator_current_bump(allocator));
}
ptrdiff_t cecs_arena_allocator_current_bump_available_aligned(const cecs_arena_allocator *allocator, const size_t alignment) {
    return cecs_bump_view_allocator_available_aligned(cecs_arena_allocator_current_bump(allocator), alignment);
}
