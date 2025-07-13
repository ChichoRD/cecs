#ifndef CECS_ALLOCATION_H
#define CECS_ALLOCATION_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

typedef void *restrict cecs_alloc(const size_t size);
typedef void *restrict cecs_realloc(void *block, const size_t block_size, const size_t new_size);
typedef void cecs_free(void *block, const size_t block_size);

typedef struct cecs_raw_alloction {
    void *restrict block;
} cecs_raw_alloction;
inline bool cecs_raw_alloction_check(const cecs_raw_alloction allocation) {
    return allocation.block != NULL;
}
inline void *restrict cecs_raw_alloction_look(const cecs_raw_alloction allocation) {
    return allocation.block;
}
inline void *restrict cecs_raw_alloction_expect(const cecs_raw_alloction allocation) {
    if (!cecs_raw_alloction_check(allocation)) {
        assert(false && "fatal error: allocation failed");
        exit(EXIT_FAILURE);
    }
    return cecs_raw_alloction_look(allocation);
}


#ifndef CECS_ALLOC_FUNC_IS_ZERO_INIT
#define CECS_ALLOC_FUNC_IS_ZERO_INIT_DEFAULT true

#define CECS_ALLOC_FUNC_IS_ZERO_INIT CECS_ALLOC_FUNC_IS_ZERO_INIT_DEFAULT
#endif


cecs_raw_alloction cecs_alloc_raw(const size_t size);
cecs_raw_alloction cecs_realloc_raw(const cecs_raw_alloction block, const size_t block_size, const size_t new_size);
void cecs_free_raw(const cecs_raw_alloction block, const size_t block_size);


void *restrict cecs_alloc_expect(const size_t size);
void *restrict cecs_realloc_expect(void *block, const size_t block_size, const size_t new_size);
void cecs_free_expect(void *block, const size_t block_size);


typedef void *restrict cecs_allocator_alloc(void *allocator, const size_t size);
typedef void *restrict cecs_allocator_alloc_aligned(void *allocator, const size_t size, const size_t alignment);
typedef void *restrict cecs_allocator_realloc(void *allocator, void *block, const size_t block_size, const size_t new_size);
typedef void *restrict cecs_allocator_realloc_aligned(
    void *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
typedef void cecs_allocator_free(void *allocator, void *block, const size_t block_size);


size_t cecs_max_alignment_from_size(const size_t size);
inline const uint8_t *cecs_aligned_ptr(const uint8_t *const address, const size_t alignment) {
    assert(cecs_is_pow2(alignment) && "fatal error: alignment is not a power of 2");

    static_assert(sizeof(size_t) == sizeof(uintptr_t), "fatal static error: size_t is not the same size as uintptr_t");
    uint8_t *const aligned = (uint8_t *)cecs_align_to_pow2((size_t)address, alignment);
    return aligned;
}
inline uint8_t *cecs_aligned_ptr_mut(uint8_t *const address, const size_t alignment) {
    assert(cecs_is_pow2(alignment) && "fatal error: alignment is not a power of 2");

    static_assert(sizeof(size_t) == sizeof(uintptr_t), "fatal static error: size_t is not the same size as uintptr_t");
    uint8_t *const aligned = (uint8_t *)cecs_align_to_pow2((size_t)address, alignment);
    return aligned;
}

#endif