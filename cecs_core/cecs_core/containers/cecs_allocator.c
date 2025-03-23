#include "cecs_allocator.h"

void *restrict cecs_allocator_alloc_aligned(cecs_allocator *allocator, const size_t size, const size_t alignment) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        return cecs_bump_allocator_alloc_aligned_expect(&allocator->allocator.bump, size, alignment);
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void *restrict cecs_allocator_alloc(cecs_allocator *allocator, const size_t size) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        return cecs_bump_allocator_alloc_expect(&allocator->allocator.bump, size);
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void *restrict cecs_allocator_realloc_aligned(
    cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        return cecs_bump_allocator_realloc_aligned_expect(&allocator->allocator.bump, block, block_size, new_size, alignment);
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void *restrict cecs_allocator_realloc(cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        return cecs_bump_allocator_realloc_expect(&allocator->allocator.bump, block, block_size, new_size);
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void cecs_allocator_free(cecs_allocator *allocator, void *block, const size_t block_size) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        cecs_bump_allocator_free(&allocator->allocator.bump, block, block_size);
        break;
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void cecs_allocator_reset(cecs_allocator *allocator) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        cecs_allocator_reset(&allocator->allocator.bump);
        break;
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}

void cecs_allocator_destroy(cecs_allocator *allocator) {
    switch (allocator->type) {
    case cecs_internal_allocator_bump:
        cecs_bump_allocator_destroy(&allocator->allocator.bump);
        break;
    default: {
        assert(false && "fatal error: allocator type is not supported");
        exit(EXIT_FAILURE);
    }
    }
}
