#include "cecs_bump_allocator.h"
#include <cecs_core/cecs_error.h>

#include <cecs_math/relations/cecs_ordering.h>

extern inline cecs_bump_view_allocator cecs_bump_view_allocator_create(const cecs_memory_block block);
void *cecs_bump_view_allocator_alloc_aligned_expect(cecs_bump_view_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_assert_or_exit(allocator->next != NULL, "fatal error: allocator is empty");
    cecs_assert_or_exit(cecs_is_pow2(alignment), "fatal error: alignment is not a power of 2");

    static_assert(sizeof(size_t) == sizeof(uintptr_t), "fatal error: size_t is not the same size as uintptr_t");
    uint8_t *const aligned = cecs_aligned_ptr_mut(allocator->next, alignment);
    uint8_t *const next = aligned + size;

    if (cecs_expect_not(next > allocator->block.memory_end)) {
        cecs_assert_or_exit(
            next <= allocator->block.memory_start + allocator->block.reserved,
            "fatal error: allocator is out of memory"
        );
        const size_t needed = (size_t)(next - allocator->block.memory_end);
        const size_t heuristic =
            cecs_min(cecs_memory_block_uncommited_size(allocator->block), cecs_memory_block_committed_size(allocator->block));
        
        const size_t commit_size = cecs_max(needed, heuristic);
        cecs_memory_block_commit_expect(&allocator->block, commit_size, allocator->block.memory_end);
    }

    allocator->next = next;
    return aligned;
}

void *cecs_bump_view_allocator_alloc_expect(cecs_bump_view_allocator *allocator, const size_t size) {
    return cecs_bump_view_allocator_alloc_aligned_expect(
        allocator,
        size,
        cecs_max_alignment_from_size(size)
    );
}

void *cecs_bump_view_allocator_realloc_aligned_expect(
    cecs_bump_view_allocator *allocator, void *const block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    cecs_assert_or_exit(allocator->next != NULL, "fatal error: allocator is empty");
    cecs_assert_or_exit(cecs_is_pow2(alignment), "fatal error: alignment is not a power of 2");

    uint8_t *const old_block = (uint8_t *)block;
    if (old_block + block_size == allocator->next) {
        cecs_assert_or_exit(
            cecs_is_aligned_to_pow2((size_t)old_block, alignment),
            "fatal error: block is not aligned to alignment"
        );
        
        uint8_t *const next = old_block + new_size;
        if (next <= allocator->block.memory_end) {
            allocator->next = next;
            return old_block;
        } else {
            cecs_assert_or_exit(
                next <= allocator->block.memory_start + allocator->block.reserved,
                "fatal error: allocator is out of memory"
            );
    
            const size_t needed = (size_t)(next - allocator->block.memory_end);
            const size_t heuristic =
                cecs_min(cecs_memory_block_uncommited_size(allocator->block), cecs_memory_block_committed_size(allocator->block));
            
            const size_t commit_size = cecs_max(needed, heuristic);
            cecs_memory_block_commit_expect(&allocator->block, commit_size, allocator->block.memory_end);
    
            allocator->next = next;
            return old_block;
        }
    } else {
        void *const new_block = cecs_bump_view_allocator_alloc_aligned_expect(allocator, new_size, alignment);
        memcpy(new_block, old_block, block_size);
        return new_block;
    }
}

void *cecs_bump_view_allocator_realloc_expect(
    cecs_bump_view_allocator *allocator, void *const block, const size_t block_size, const size_t new_size
) {
    return cecs_bump_view_allocator_realloc_aligned_expect(
        allocator,
        block,
        block_size,
        new_size,
        cecs_max_alignment_from_size(new_size)
    );
}
void cecs_bump_view_allocator_free(cecs_bump_view_allocator *allocator, void *const block, const size_t block_size) {
    if (cecs_expect_not((uint8_t *)block + block_size == allocator->next)) {
        allocator->next = (uint8_t *)block;
    }
}
extern inline uint8_t *cecs_bump_view_allocator_snapshot(cecs_bump_view_allocator *allocator);
extern inline void cecs_bump_view_allocator_reset_to(cecs_bump_view_allocator *allocator, uint8_t *const snapshot, uint8_t *const end_snapshot);
extern inline void cecs_bump_view_allocator_reset(cecs_bump_view_allocator *allocator);
extern inline void cecs_bump_view_allocator_destroy(cecs_bump_view_allocator *allocator);

extern inline size_t cecs_bump_view_allocator_capacity(const cecs_bump_view_allocator *allocator);
extern inline size_t cecs_bump_view_allocator_used(const cecs_bump_view_allocator *allocator);
extern inline ptrdiff_t cecs_bump_view_allocator_available(const cecs_bump_view_allocator *allocator);
ptrdiff_t cecs_bump_view_allocator_available_aligned(const cecs_bump_view_allocator *allocator, const size_t alignment) {
    cecs_assert_or_exit(allocator->next != NULL, "fatal error: allocator is empty");
    const uint8_t *const aligned = cecs_aligned_ptr(allocator->next, alignment);
    return (ptrdiff_t)((allocator->block.memory_start + allocator->block.reserved) - aligned);
}


extern inline cecs_bump_allocator cecs_bump_allocator_from_view(cecs_bump_view_allocator view);
cecs_bump_allocator cecs_bump_allocator_create_alloc(const size_t block_size) {
    return cecs_bump_allocator_from_view(
        cecs_bump_view_allocator_create(cecs_alloc_block_expect(block_size))
    );
}
cecs_bump_allocator cecs_bump_allocator_create_virtual(const size_t page_count) {
    const size_t size = page_count * cecs_system_page_size();
    return cecs_bump_allocator_from_view(
        cecs_bump_view_allocator_create(cecs_memory_block_map_expect(size))
    );
}

extern inline void *cecs_bump_allocator_alloc_aligned_expect(cecs_bump_allocator *allocator, const size_t size, const size_t alignment);
extern inline void *cecs_bump_allocator_alloc_expect(cecs_bump_allocator *allocator, const size_t size);

extern inline void *cecs_bump_allocator_realloc_aligned_expect(
    cecs_bump_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
extern inline void *cecs_bump_allocator_realloc_expect(
    cecs_bump_allocator *allocator, void *block, const size_t block_size, const size_t new_size
);

extern inline void cecs_bump_allocator_free(cecs_bump_allocator *allocator, void *block, const size_t block_size);
extern inline uint8_t *cecs_bump_allocator_snapshot(cecs_bump_allocator *allocator);
extern inline void cecs_bump_allocator_reset_to(cecs_bump_allocator *allocator, uint8_t *const snapshot, uint8_t *const end_snapshot);
extern inline void cecs_bump_allocator_reset(cecs_bump_allocator *allocator);

extern inline size_t cecs_bump_allocator_capacity(const cecs_bump_allocator *allocator);
extern inline size_t cecs_bump_allocator_used(const cecs_bump_allocator *allocator);
extern inline ptrdiff_t cecs_bump_allocator_available(const cecs_bump_allocator *allocator);
extern inline ptrdiff_t cecs_bump_allocator_available_aligned(const cecs_bump_allocator *allocator, const size_t alignment);
