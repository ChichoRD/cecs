#include "cecs_bump_allocator.h"
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>

extern inline cecs_bump_view_allocator cecs_bump_view_allocator_create(void *block_start, void *block_end);
void *restrict cecs_bump_view_allocator_alloc_aligned_expect(cecs_bump_view_allocator *allocator, const size_t size, const size_t alignment) {
    assert(allocator->next != NULL && "fatal error: allocator is empty");
    assert(cecs_is_pow2(alignment) && "fatal error: alignment is not a power of 2");

    static_assert(sizeof(size_t) == sizeof(uintptr_t), "fatal error: size_t is not the same size as uintptr_t");
    uint8_t *const aligned = (uint8_t *)cecs_align_to_pow2((size_t)allocator->next, alignment);
    uint8_t *const next = aligned + size;

    if (next > allocator->block_end) {
        assert(false && "fatal error: allocator is out of memory");
        exit(EXIT_FAILURE);
    }

    allocator->next = next;
    return aligned;
}

void *restrict cecs_bump_view_allocator_alloc_expect(cecs_bump_view_allocator *allocator, const size_t size) {
    return cecs_bump_view_allocator_alloc_aligned_expect(
        allocator,
        size,
        cecs_max_alignment_from_size(size)
    );
}

void *restrict cecs_bump_view_allocator_realloc_aligned_expect(
    cecs_bump_view_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    assert(allocator->next != NULL && "fatal error: allocator is empty");
    assert(cecs_is_pow2(alignment) && "fatal error: alignment is not a power of 2");

    uint8_t *const old_block = (uint8_t *)block;
    if (old_block + block_size == allocator->next) {
        assert(cecs_is_aligned_to_pow2((size_t)old_block, alignment) && "fatal error: block is not aligned to alignment");
        uint8_t *const next = old_block + new_size;
        if (next > allocator->block_end) {
            assert(false && "fatal error: allocator is out of memory");
            exit(EXIT_FAILURE);
        }
        allocator->next = next;
        return old_block;
    } else {
        return cecs_bump_view_allocator_alloc_aligned_expect(allocator, new_size, alignment);
    }
}

void *restrict cecs_bump_view_allocator_realloc_expect(
    cecs_bump_view_allocator *allocator, void *block, const size_t block_size, const size_t new_size
) {
    return cecs_bump_view_allocator_realloc_aligned_expect(
        allocator,
        block,
        block_size,
        new_size,
        cecs_max_alignment_from_size(new_size)
    );
}

extern inline void cecs_bump_view_allocator_free(cecs_bump_view_allocator *allocator, void *block, const size_t block_size);
extern inline void cecs_bump_view_allocator_reset(cecs_bump_view_allocator *allocator);
extern inline void cecs_bump_view_allocator_destroy(cecs_bump_view_allocator *allocator);

extern inline size_t cecs_bump_view_allocator_capacity(const cecs_bump_view_allocator *allocator);
extern inline size_t cecs_bump_view_allocator_used(const cecs_bump_view_allocator *allocator);
extern inline ptrdiff_t cecs_bump_view_allocator_available(const cecs_bump_view_allocator *allocator);
ptrdiff_t cecs_bump_view_allocator_available_aligned(const cecs_bump_view_allocator *allocator, const size_t alignment) {
    assert(allocator->next != NULL && "fatal error: allocator is empty");
    assert(cecs_is_pow2(alignment) && "fatal error: alignment is not a power of 2");

    static_assert(sizeof(size_t) == sizeof(uintptr_t), "fatal error: size_t is not the same size as uintptr_t");
    uint8_t *const aligned = (uint8_t *)cecs_align_to_pow2((size_t)allocator->next, alignment);
    return (ptrdiff_t)(allocator->block_end - aligned);
}


cecs_bump_allocator cecs_bump_allocator_create(const size_t block_size) {
    uint8_t *const block = cecs_alloc_expect(block_size);
    return (cecs_bump_allocator){
        .view = cecs_bump_view_allocator_create(block, block + block_size)
    };
}

extern inline void *restrict cecs_bump_allocator_alloc_aligned_expect(cecs_bump_allocator *allocator, const size_t size, const size_t alignment);
extern inline void *restrict cecs_bump_allocator_alloc_expect(cecs_bump_allocator *allocator, const size_t size);

extern inline void *restrict cecs_bump_allocator_realloc_aligned_expect(
    cecs_bump_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
extern inline void *restrict cecs_bump_allocator_realloc_expect(
    cecs_bump_allocator *allocator, void *block, const size_t block_size, const size_t new_size
);

extern inline void cecs_bump_allocator_free(cecs_bump_allocator *allocator, void *block, const size_t block_size);
extern inline void cecs_bump_allocator_reset(cecs_bump_allocator *allocator);

extern inline size_t cecs_bump_allocator_capacity(const cecs_bump_allocator *allocator);
extern inline size_t cecs_bump_allocator_used(const cecs_bump_allocator *allocator);
extern inline ptrdiff_t cecs_bump_allocator_available(const cecs_bump_allocator *allocator);
extern inline ptrdiff_t cecs_bump_allocator_available_aligned(const cecs_bump_allocator *allocator, const size_t alignment);
