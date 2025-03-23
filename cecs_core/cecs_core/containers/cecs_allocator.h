#ifndef CECS_ALLOCATOR_H
#define CECS_ALLOCATOR_H

#include "allocator/cecs_bump_allocator.h"

typedef union cecs_internal_allocator {
    cecs_bump_allocator bump;
} cecs_internal_allocator;
typedef enum cecs_internal_allocator_type {
    cecs_internal_allocator_bump
} cecs_internal_allocator_type;

typedef struct cecs_allocator {
    cecs_internal_allocator allocator;
    cecs_internal_allocator_type type;
} cecs_allocator;


inline cecs_allocator cecs_allocator_create_from(const cecs_internal_allocator allocator, const cecs_internal_allocator_type type) {
    return (cecs_allocator){
        .allocator = allocator,
        .type = type
    };
}

void *restrict cecs_allocator_alloc_aligned(cecs_allocator *allocator, const size_t size, const size_t alignment);
void *restrict cecs_allocator_alloc(cecs_allocator *allocator, const size_t size);

void *restrict cecs_allocator_realloc_aligned(cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment);
void *restrict cecs_allocator_realloc(cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size);

void cecs_allocator_free(cecs_allocator *allocator, void *block, const size_t block_size);
void cecs_allocator_reset(cecs_allocator *allocator);

void cecs_allocator_destroy(cecs_allocator *allocator);

#endif