#ifndef CECS_ALLOCATION_H
#define CECS_ALLOCATION_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include "cecs_memory.h"

typedef void *cecs_alloc_fn(const size_t size);
typedef void *cecs_realloc_fn(void *block, const size_t block_size, const size_t new_size);
typedef void cecs_free_fn(void *block, const size_t block_size);

typedef struct cecs_raw_alloction {
    void *block;
} cecs_raw_alloction;
inline bool cecs_raw_alloction_check(const cecs_raw_alloction allocation) {
    return allocation.block != NULL;
}
inline void *cecs_raw_alloction_look(const cecs_raw_alloction allocation) {
    return allocation.block;
}
inline void *cecs_raw_alloction_expect(const cecs_raw_alloction allocation) {
    cecs_assert_or_exit(
        cecs_raw_alloction_check(allocation),
        "fatal error: allocation failed"
    );
    return cecs_raw_alloction_look(allocation);
}


#ifndef CECS_ALLOC_FUNC_IS_ZERO_INIT
#define CECS_ALLOC_FUNC_IS_ZERO_INIT_DEFAULT true

#define CECS_ALLOC_FUNC_IS_ZERO_INIT CECS_ALLOC_FUNC_IS_ZERO_INIT_DEFAULT
#endif


cecs_raw_alloction cecs_alloc_raw(const size_t size);
cecs_raw_alloction cecs_realloc_raw(const cecs_raw_alloction block, const size_t block_size, const size_t new_size);
void cecs_free_raw(const cecs_raw_alloction block, const size_t block_size);


void *cecs_alloc_expect(const size_t size);
void *cecs_realloc_expect(void *block, const size_t block_size, const size_t new_size);
void cecs_free_expect(void *block, const size_t block_size);


cecs_memory_block cecs_alloc_block_expect(const size_t size);
cecs_memory_block cecs_realloc_block_expect(cecs_memory_block *block, const size_t block_size, const size_t new_size);
void cecs_free_block_expect(cecs_memory_block block, const size_t block_size);


typedef void *cecs_allocator_alloc_fn(void *allocator, const size_t size);
typedef void *cecs_allocator_alloc_aligned_fn(void *allocator, const size_t size, const size_t alignment);
typedef void *cecs_allocator_realloc_fn(void *allocator, void *block, const size_t block_size, const size_t new_size);
typedef void *cecs_allocator_realloc_aligned_fn(
    void *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
typedef void cecs_allocator_free_fn(void *allocator, void *block, const size_t block_size);


#endif
