#include "cecs_pool_allocator.h"
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>
#include <memory.h>

static cecs_pool_allocator cecs_pool_allocator_create(
    const size_t block_size,
    const cecs_arena_allocator_bump_index blocks_capacity,
    const uint_fast8_t granularity_log2
) {
    assert(block_size > 0 && "fatal error: block_size must be greater than 0");
    assert(blocks_capacity > 0 && "fatal error: blocks_capacity must be greater than 0");
    assert(
        cecs_is_aligned_to_pow2((size_t)block_size, (size_t)(1 << granularity_log2))
        && "fatal error: block_size must be aligned to granularity"
    );
    
    const size_t grains_per_block = block_size >> granularity_log2;
    const size_t masks_per_block = grains_per_block / (CHAR_BIT * sizeof(cecs_pool_allocator_mask));
    const size_t masks_capacity = masks_per_block * blocks_capacity;

    const size_t total_mask_size = masks_capacity * sizeof(cecs_pool_allocator_mask);
    cecs_pool_allocator_mask *const free_mask = cecs_alloc_expect(total_mask_size);
#if !CECS_ALLOC_FUNC_IS_ZERO_INIT
    memset(free_mask, 0, total_mask_size);
#endif

    return (cecs_pool_allocator){
        .arena = cecs_arena_allocator_create(block_size, blocks_capacity),
        .free_mask = free_mask,
        .first_free_mask_index = 0,
        .granularity_log2 = granularity_log2
    };
}
cecs_pool_allocator cecs_pool_allocator_create_u64(const size_t block_size, const cecs_arena_allocator_bump_index blocks_capacity) {
    return cecs_pool_allocator_create(block_size, blocks_capacity, 6);
}

cecs_pool_allocator cecs_pool_allocator_create_u128(const size_t block_size, const cecs_arena_allocator_bump_index blocks_capacity) {
    return cecs_pool_allocator_create(block_size, blocks_capacity, 7);
}

cecs_pool_allocator cecs_pool_allocator_create_u256(const size_t block_size, const cecs_arena_allocator_bump_index blocks_capacity) {
    return cecs_pool_allocator_create(block_size, blocks_capacity, 8);
}

typedef struct cecs_pool_allocator_mask_proxy {
    size_t mask_index;
    uint8_t mask_start_bit;
    uint8_t mask_length;
} cecs_pool_allocator_mask_proxy;

static cecs_pool_allocator_mask_proxy cecs_pool_allocator_find_free(
    const cecs_pool_allocator *allocator,
    const size_t size
) {
    assert(size > 0 && "fatal error: size must be greater than 0");
    const size_t mask_length = (size + (1 << allocator->granularity_log2) - 1) >> allocator->granularity_log2;

    for (size_t i = allocator->first_free_mask_index; i < allocator->arena.bump_capacity; i++) {
        const cecs_pool_allocator_mask mask = allocator->free_mask[i];
        const cecs_pool_allocator_mask mask_run = cecs_mark_bit_runs(mask, mask_length);

        if (mask_run != 0) {
            const cecs_pool_allocator_mask_proxy mask_proxy = {
                .mask_index = i,
                .mask_start_bit = cecs_log2(mask_run & -mask_run),
                .mask_length = mask_length
            };
            return mask_proxy;
        }
    }

    return (cecs_pool_allocator_mask_proxy){0};
}

static void cecs_pool_allocator_grow_mask(cecs_pool_allocator *allocator, const size_t new_arena_bump_index) {

}

void *restrict cecs_pool_allocator_alloc_aligned(cecs_pool_allocator *allocator, const size_t size, const size_t alignment) {
    // if (cecs_arena_allocator_current_bump_available_aligned(&allocator->arena, alignment) < (ptrdiff_t)size) {
    //     cecs_pool_allocator_mask_proxy mask_proxy = cecs_pool_allocator_find_free(allocator, size);
    //     if (mask_proxy.mask_length == 0){
    //         const size_t previous_bump_capacity = allocator->arena.bump_capacity;
    //         void *const restrict block = cecs_arena_allocator_alloc_aligned(&allocator->arena, size, alignment);
    //         if (allocator->arena.bump_capacity > previous_bump_capacity) {
    //             // const size_t new_masks_capacity = allocator->arena.bump_capacity * (block_size >> allocator->granularity_log2) / (CHAR_BIT * sizeof(cecs_pool_allocator_mask));
    //             const size_t new_total_mask_size = new_masks_capacity * sizeof(cecs_pool_allocator_mask);
    //             cecs_pool_allocator_mask *const new_free_mask = cecs_alloc_expect(new_total_mask_size);
    //     } else {
    //         allocator->free_mask[mask_proxy.mask_index] &= ~cecs_bitmask(mask_proxy.mask_start_bit, mask_proxy.mask_length);
    //         if (allocator->free_mask[mask_proxy.mask_index] == 0) {
    //             allocator->first_free_mask_index = mask_proxy.mask_index + 1;
    //         }

    //         size_t bytes_to_skip = mask_proxy.mask_index << allocator->granularity_log2;
    //         cecs_bump_allocator *current_bump = &allocator->arena.bumps[0];
    //         size_t current_bump_capacity = cecs_bump_allocator_capacity(current_bump);
    //         while (bytes_to_skip >= current_bump_capacity) {
    //             bytes_to_skip -= current_bump_capacity;

    //             ++current_bump;
    //             current_bump_capacity = cecs_bump_allocator_capacity(current_bump);
    //         }

    //         void *const restrict block = current_bump->block_start + bytes_to_skip;
    //         assert(cecs_is_aligned_to_pow2((size_t)block, alignment) && "fatal error: block is not aligned to alignment");
    //         return block;
    //     }
    // } else {
    //     return cecs_arena_allocator_alloc_aligned(&allocator->arena, size, alignment);
    // }
}

void *restrict cecs_pool_allocator_alloc(cecs_pool_allocator *allocator, const size_t size) {
    return cecs_pool_allocator_alloc_aligned(allocator, size, cecs_max_alignment_from_size(size));
}

void *restrict cecs_pool_allocator_realloc_aligned(cecs_pool_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment) {
    if (cecs_arena_allocator_current_bump_available_aligned(&allocator->arena, alignment) < (ptrdiff_t)new_size) {
        cecs_pool_allocator_free(allocator, block, block_size);
        return cecs_pool_allocator_alloc_aligned(allocator, new_size, alignment);
    } else {
        return cecs_arena_allocator_realloc_aligned(&allocator->arena, block, block_size, new_size, alignment);
    }
}

void *restrict cecs_pool_allocator_realloc(cecs_pool_allocator *allocator, void *block, const size_t block_size, const size_t new_size) {
    return cecs_pool_allocator_realloc_aligned(allocator, block, block_size, new_size, cecs_max_alignment_from_size(new_size));
}

void cecs_pool_allocator_free(cecs_pool_allocator *allocator, void *block, const size_t block_size) {
    const cecs_bump_allocator *current_bump = &allocator->arena.bumps[0];
    const void *current_next = current_bump->next;

    size_t i = 0;
    size_t skipped_bytes = 0;
    while (
        (block < current_bump->block_start || block >= current_next)
        && i < allocator->arena.bump_capacity
    ) {
        ++current_bump;
        current_next = current_bump->next;
        
        ++i;
        skipped_bytes += cecs_bump_allocator_capacity(current_bump);
    }

    if (i == allocator->arena.bump_capacity) {
        assert(false && "fatal error: block is not in any bump");
        exit(EXIT_FAILURE);
    } else {
        assert(i < allocator->arena.bump_capacity && "fatal error: block is not in any bump");
        assert(block >= current_bump->block_start && block < current_next && "fatal error: block is not in bump");

        const size_t bump_offset = (uint8_t *)block - current_bump->block_start;
        skipped_bytes += bump_offset;

        const size_t mask_global_bit = skipped_bytes >> allocator->granularity_log2;
        const size_t mask_index = mask_global_bit / (CHAR_BIT * sizeof(cecs_pool_allocator_mask));
        const uint_fast8_t mask_start_bit = (bump_offset >> allocator->granularity_log2) & (CHAR_BIT * sizeof(cecs_pool_allocator_mask) - 1);
        const uint_fast8_t mask_length = (block_size + (1 << allocator->granularity_log2) - 1) >> allocator->granularity_log2;

        allocator->free_mask[mask_index] |= cecs_bitmask(mask_start_bit, mask_length);
        if (mask_index < allocator->first_free_mask_index) {
            allocator->first_free_mask_index = mask_index;
        }
    }
}

void cecs_pool_allocator_reset(cecs_pool_allocator *allocator) {
    cecs_arena_allocator_reset(&allocator->arena);
    memset(allocator->free_mask, 0, allocator->arena.bump_capacity * sizeof(cecs_pool_allocator_mask));
    allocator->first_free_mask_index = 0;   
}

void cecs_pool_allocator_destroy(cecs_pool_allocator *allocator) {
    cecs_free_expect(allocator->free_mask, allocator->arena.bump_capacity * sizeof(cecs_pool_allocator_mask));
    cecs_arena_allocator_destroy(&allocator->arena);
}
