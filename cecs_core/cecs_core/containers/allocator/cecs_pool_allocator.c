#include "cecs_pool_allocator.h"
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>
#include <memory.h>

cecs_pool_allocator cecs_pool_allocator_create(const size_t block_size, const cecs_pool_allocator_block_usize blocks_capacity) {
    return (cecs_pool_allocator){
        .arena = cecs_arena_allocator_create(block_size, blocks_capacity),
        .free_blocks = NULL,
        .free_block_refs = cecs_alloc_expect(blocks_capacity * sizeof(cecs_pool_allocator_free_block_ref)),
        .max_free_block_size = 0,
        .free_blocks_count = 0
    };
}

static bool cecs_pool_allocator_find_free_block(
    cecs_pool_allocator *allocator, const size_t aligned_size, cecs_pool_allocator_block_usize *out_block_ref_index
) {
    assert(aligned_size > 0 && "fatal error: aligned_size must be greater than 0");
    if (allocator->free_blocks_count == 0 || aligned_size > allocator->max_free_block_size) {
        return false;
    }

    const size_t max_size_log2 = cecs_log2(aligned_size);
    cecs_pool_allocator_block_usize block_ref_index = 0;
    bool found = false;
    while (block_ref_index < allocator->free_blocks_count && !found) {
        const cecs_pool_allocator_free_block_ref ref = allocator->free_block_refs[block_ref_index];
        if (ref.max_size_log2 >= max_size_log2) {
            found = true;
            *out_block_ref_index = block_ref_index;
        } else {
            block_ref_index++;
        }
    }
    return found;
}

void *restrict cecs_pool_allocator_alloc_aligned(cecs_pool_allocator *allocator, const size_t size, const size_t alignment) {
    if (cecs_arena_allocator_current_bump_available_aligned(&allocator->arena, alignment) < (ptrdiff_t)size) {
        const size_t aligned_size = cecs_align_to_pow2(size, alignment);
        cecs_pool_allocator_block_usize ref_index;
        if (cecs_pool_allocator_find_free_block(allocator, aligned_size, &ref_index)) {
            cecs_pool_allocator_free_block_ref *ref = &allocator->free_block_refs[ref_index];
            assert(ref->index < allocator->free_blocks_count && "fatal error: invalid free block index");

            cecs_pool_allocator_free_block *block = &allocator->free_blocks[ref->index];
            assert(block->size >= size && "fatal error: free block size is less than requested size");

            void *const restrict block = block->block;
            const cecs_pool_allocator_block_usize next_largest_block_index = block->next_largest_block_index;
            const size_t next_largest_block_size = allocator->free_blocks[next_largest_block_index].size;
            const uint_fast8_t next_largest_block_size_log2 = cecs_log2(next_largest_block_size);
            static_assert(false, "TODO");
            if (aligned_size < block->size) {
                void *const restrict advanced_block = block + aligned_size;
                const size_t advanced_block_size = block->size - aligned_size;

                block->size = aligned_size;
                block->block = advanced_block;

                if (advanced_block_size < next_largest_block_size) {
                    ref->index = next_largest_block_index;
                    ref->max_size_log2 = next_largest_block_size_log2;
                }
            } else {
                ref->index = next_largest_block_index;
                ref->max_size_log2 = next_largest_block_size_log2;
            }

            return block;
        } else {
            return cecs_arena_allocator_alloc_aligned(&allocator->arena, size, alignment);
        }
    } else {
        return cecs_arena_allocator_alloc_aligned(&allocator->arena, size, alignment);
    }
}

void *restrict cecs_pool_allocator_alloc(cecs_pool_allocator *allocator, const size_t size)
{
    return void *restrict();
}

void *restrict cecs_pool_allocator_realloc_aligned(cecs_pool_allocator * allocator, void * block, const size_t block_size, const size_t new_size, const size_t alignment)
{
return void *restrict();
}

void *restrict cecs_pool_allocator_realloc(cecs_pool_allocator *allocator, void *block, const size_t block_size, const size_t new_size)
{
    return void *restrict();
}

void cecs_pool_allocator_free(cecs_pool_allocator *allocator, void *block, const size_t size)
{
}

void cecs_pool_allocator_reset(cecs_pool_allocator *allocator)
{
}

void cecs_pool_allocator_destroy(cecs_pool_allocator *allocator)
{
}
