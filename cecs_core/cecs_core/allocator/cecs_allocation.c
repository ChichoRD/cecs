#include "cecs_allocation.h"
#include <cecs_core/cecs_error.h>

#include <stdint.h>
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>
#include <cecs_math/relations/cecs_ordering.h>

extern inline bool cecs_raw_alloction_check(const cecs_raw_alloction allocation);
extern inline void *cecs_raw_alloction_look(const cecs_raw_alloction allocation);
void *cecs_raw_alloction_expect(const cecs_raw_alloction allocation) {
    cecs_assert_or_exit(
        cecs_raw_alloction_check(allocation),
        "fatal error: allocation failed"
    );
    return cecs_raw_alloction_look(allocation);
}


#ifndef CECS_ALLOC_FUNC
#define CECS_ALLOC_FUNC_DEFAULT calloc

#undef CECS_ALLOC_FUNC_HAS_CALLOC_SIGNATURE
#define CECS_ALLOC_FUNC_HAS_CALLOC_SIGNATURE true

#define CECS_ALLOC_FUNC CECS_ALLOC_FUNC_DEFAULT
#endif

#if !defined(CECS_ALLOC_FUNC_HAS_CALLOC_SIGNATURE)
static_assert(false, "static error: CECS_ALLOC_FUNC_HAS_CALLOC_SIGNATURE must be defined");
#endif

static_assert(
    !CECS_ALLOC_FUNC_HAS_CALLOC_SIGNATURE || CECS_ALLOC_FUNC_IS_ZERO_INIT,
    "static error: CECS_ALLOC_FUNC must be zero initialized if CECS_ALLOC_FUNC_HAS_CALLOC_SIGNATURE is true"
);


#ifndef CECS_REALLOC_FUNC
#define CECS_REALLOC_FUNC_DEFAULT realloc

#define CECS_REALLOC_FUNC CECS_REALLOC_FUNC_DEFAULT
#endif

#ifndef CECS_FREE_FUNC
#define CECS_FREE_FUNC_DEFAULT free

#define CECS_FREE_FUNC CECS_FREE_FUNC_DEFAULT
#endif


cecs_raw_alloction cecs_alloc_raw(const size_t size) {
    return (cecs_raw_alloction){
#if CECS_ALLOC_FUNC_HAS_CALLOC_SIGNATURE
        .block = CECS_ALLOC_FUNC(size, sizeof(uint8_t))
#else
        .block = CECS_ALLOC_FUNC(size)
#endif
    };
}
cecs_raw_alloction cecs_realloc_raw(const cecs_raw_alloction block, const size_t block_size, const size_t new_size) {
    (void)block_size;
    return (cecs_raw_alloction){
        .block = CECS_REALLOC_FUNC(block.block, new_size)
    };
}
void cecs_free_raw(const cecs_raw_alloction block, const size_t block_size) {
    (void)block_size;
    CECS_FREE_FUNC(block.block);
}


void *cecs_alloc_expect(const size_t size) {
    return cecs_raw_alloction_expect(cecs_alloc_raw(size));
}
void *cecs_realloc_expect(void *block, const size_t block_size, const size_t new_size) {
    return cecs_raw_alloction_expect(cecs_realloc_raw((cecs_raw_alloction){ .block = block }, block_size, new_size));
}
void cecs_free_expect(void *block, const size_t block_size) {
    cecs_free_raw((cecs_raw_alloction){ .block = block }, block_size);
}

cecs_memory_block cecs_alloc_block_expect(const size_t size) {
    uint8_t *const allocation = (uint8_t *)cecs_alloc_expect(size);
    return (cecs_memory_block){
        .memory_start = allocation,
        .memory_end = allocation + size,
        .reserved = size
    };
}
cecs_memory_block cecs_realloc_block_expect(cecs_memory_block *block, const size_t new_size) {
    uint8_t *const allocation = (uint8_t *)cecs_realloc_expect(block->memory_start, block->memory_end - block->memory_start, new_size);
    *block = (cecs_memory_block){0};
    return (cecs_memory_block){
        .memory_start = allocation,
        .memory_end = allocation + new_size,
        .reserved = new_size
    };
}

void cecs_free_block_expect(cecs_memory_block *block, const size_t block_size) {
    cecs_free_expect(block->memory_start, block_size);
    *block = (cecs_memory_block){0};
}
