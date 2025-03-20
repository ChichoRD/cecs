#ifndef CECS_BUMP_ALLOCATOR_H
#define CECS_BUMP_ALLOCATOR_H

#include "cecs_allocation.h"
#include <stdint.h>

typedef struct cecs_bump_allocator {
    uint8_t *next;
    uint8_t *const block_start;
    uint8_t *const block_end;
} cecs_bump_allocator;

inline cecs_bump_allocator cecs_bump_allocator_create_empty(void) {
    return (cecs_bump_allocator){
        .next = NULL,
        .block_start = NULL,
        .block_end = NULL
    };
}
cecs_bump_allocator cecs_bump_allocator_create(const size_t block_size);
inline cecs_bump_allocator cecs_bump_allocator_create_from(void *block_start, void *block_end) {
    assert(block_start != NULL && "fatal error: block_start is NULL");
    assert(block_end != NULL && "fatal error: block_end is NULL");
    assert(block_start < block_end && "fatal error: block_end must be strictly greater than block_start");
    return (cecs_bump_allocator){
        .next = block_start,
        .block_start = block_start,
        .block_end = block_end
    };
}

void *cecs_bump_allocator_alloc_aligned_expect(cecs_bump_allocator *allocator, const size_t size, const size_t alignment);
void *cecs_bump_allocator_alloc_expect(cecs_bump_allocator *allocator, const size_t size);

void *cecs_bump_allocator_realloc_aligned_expect(
    cecs_bump_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
);
void *cecs_bump_allocator_realloc_expect(
    cecs_bump_allocator *allocator, void *block, const size_t block_size, const size_t new_size
);

inline void cecs_bump_allocator_free(cecs_bump_allocator *allocator, void *block, const size_t block_size) {
    (void)allocator;
    (void)block;
    (void)block_size;
}
inline void cecs_allocator_reset(cecs_bump_allocator *allocator) {
    assert(allocator->next != NULL && "fatal error: allocator is empty");
    allocator->next = allocator->block_start;
}
void cecs_bump_allocator_destroy(cecs_bump_allocator *allocator);


inline size_t cecs_bump_allocator_capacity(const cecs_bump_allocator *allocator) {
    assert(allocator->next != NULL && "fatal error: allocator is empty");
    return (size_t)(allocator->block_end - allocator->block_start);
}
inline size_t cecs_bump_allocator_used(const cecs_bump_allocator *allocator) {
    assert(allocator->next != NULL && "fatal error: allocator is empty");
    return (size_t)(allocator->next - allocator->block_start);
}
inline ptrdiff_t cecs_bump_allocator_available(const cecs_bump_allocator *allocator) {
    assert(allocator->next != NULL && "fatal error: allocator is empty");
    return (ptrdiff_t)(allocator->block_end - allocator->next);
}	
ptrdiff_t cecs_bump_allocator_available_aligned(const cecs_bump_allocator *allocator, const size_t alignment);
#endif