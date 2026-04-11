#ifndef CECS_BUMP_ALLOCATOR_H
#define CECS_BUMP_ALLOCATOR_H

#include "cecs_allocation.h"
#include "cecs_memory.h"
#include <cecs_error.h>
#include <stdint.h>

typedef struct cecs_bump_view_allocator {
    cecs_memory_block block;
    uint8_t *next;
} cecs_bump_view_allocator;

inline cecs_bump_view_allocator cecs_bump_view_allocator_create(const cecs_memory_block block) {
    return (cecs_bump_view_allocator){
        .block = block,
        .next = block.memory_start
    };
}

void *cecs_bump_view_allocator_alloc_aligned_expect(cecs_bump_view_allocator *allocator, const size_t size, const size_t alignment);
void *cecs_bump_view_allocator_alloc_expect(cecs_bump_view_allocator *allocator, const size_t size);

void *cecs_bump_view_allocator_realloc_aligned_expect(
    cecs_bump_view_allocator *allocator, void *const block, const size_t block_size, const size_t new_size, const size_t alignment
);
void *cecs_bump_view_allocator_realloc_expect(
    cecs_bump_view_allocator *allocator, void *const block, const size_t block_size, const size_t new_size
);

void cecs_bump_view_allocator_free(cecs_bump_view_allocator *allocator, void *const block, const size_t block_size);

inline uint8_t *cecs_bump_view_allocator_snapshot(cecs_bump_view_allocator *allocator) {
    return allocator->next;
}
inline void cecs_bump_view_allocator_reset_to(cecs_bump_view_allocator *allocator, uint8_t *const snapshot, uint8_t *const end_snapshot) {
    cecs_debugbreak_fail_unless(allocator->next != NULL, "fatal error: allocator is empty");
    cecs_debugbreak_fail_unless(end_snapshot == allocator->next, "fatal error: end snapshot does not match current allocator state");
    cecs_debugbreak_fail_unless(allocator->block.memory_start <= snapshot && snapshot <= allocator->block.memory_end, "fatal error: snapshot is out of bounds");
    allocator->next = snapshot;
}
inline void cecs_bump_view_allocator_reset(cecs_bump_view_allocator *allocator) {
    cecs_bump_view_allocator_reset_to(allocator, allocator->block.memory_start, allocator->next);
}
inline void cecs_bump_view_allocator_destroy(cecs_bump_view_allocator *allocator) {
    allocator->next = NULL;
}

inline size_t cecs_bump_view_allocator_capacity(const cecs_bump_view_allocator *allocator) {
    return allocator->block.reserved;
}
inline size_t cecs_bump_view_allocator_used(const cecs_bump_view_allocator *allocator) {
    cecs_debugbreak_fail_unless(allocator->next != NULL, "fatal error: allocator is empty");
    return (size_t)(allocator->next - allocator->block.memory_start);
}
inline ptrdiff_t cecs_bump_view_allocator_available(const cecs_bump_view_allocator *allocator) {
    cecs_debugbreak_fail_unless(allocator->next != NULL, "fatal error: allocator is empty");
    return (ptrdiff_t)((allocator->block.memory_start + allocator->block.reserved) - allocator->next);
}
ptrdiff_t cecs_bump_view_allocator_available_aligned(const cecs_bump_view_allocator *allocator, const size_t alignment);



typedef struct cecs_bump_allocator {
    cecs_bump_view_allocator view;
} cecs_bump_allocator;

inline cecs_bump_allocator cecs_bump_allocator_from_view(cecs_bump_view_allocator view) {
    return (cecs_bump_allocator){
        .view = view
    };
}
cecs_bump_allocator cecs_bump_allocator_create_alloc(const size_t block_size);
cecs_bump_allocator cecs_bump_allocator_create_virtual(const size_t page_count);

inline void *cecs_bump_allocator_alloc_aligned_expect(cecs_bump_allocator *allocator, const size_t size, const size_t alignment) {
    return cecs_bump_view_allocator_alloc_aligned_expect(&allocator->view, size, alignment);
}
inline void *cecs_bump_allocator_alloc_expect(cecs_bump_allocator *allocator, const size_t size) {
    return cecs_bump_view_allocator_alloc_expect(&allocator->view, size);
}

inline void *cecs_bump_allocator_realloc_aligned_expect(
    cecs_bump_allocator *allocator, void *const block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    return cecs_bump_view_allocator_realloc_aligned_expect(&allocator->view, block, block_size, new_size, alignment);
}
inline void *cecs_bump_allocator_realloc_expect(
    cecs_bump_allocator *allocator, void *const block, const size_t block_size, const size_t new_size
) {
    return cecs_bump_view_allocator_realloc_expect(&allocator->view, block, block_size, new_size);
}

inline void cecs_bump_allocator_free(cecs_bump_allocator *allocator, void *const block, const size_t block_size) {
    cecs_bump_view_allocator_free(&allocator->view, block, block_size);
}
inline uint8_t *cecs_bump_allocator_snapshot(cecs_bump_allocator *allocator) {
    return cecs_bump_view_allocator_snapshot(&allocator->view);
}
inline void cecs_bump_allocator_reset_to(cecs_bump_allocator *allocator, uint8_t *const snapshot, uint8_t *const end_snapshot) {
    cecs_bump_view_allocator_reset_to(&allocator->view, snapshot, end_snapshot);
}
inline void cecs_bump_allocator_reset(cecs_bump_allocator *allocator) {
    cecs_bump_view_allocator_reset(&allocator->view);
}
inline void cecs_bump_allocator_destroy(cecs_bump_allocator *allocator) {
    cecs_memory_block_unmap_expect(&allocator->view.block);
    cecs_bump_view_allocator_destroy(&allocator->view);
}


inline size_t cecs_bump_allocator_capacity(const cecs_bump_allocator *allocator) {
    return cecs_bump_view_allocator_capacity(&allocator->view);
}
inline size_t cecs_bump_allocator_used(const cecs_bump_allocator *allocator) {
    return cecs_bump_view_allocator_used(&allocator->view);
}
inline ptrdiff_t cecs_bump_allocator_available(const cecs_bump_allocator *allocator) {
    return cecs_bump_view_allocator_available(&allocator->view);
}	
inline ptrdiff_t cecs_bump_allocator_available_aligned(const cecs_bump_allocator *allocator, const size_t alignment) {
    return cecs_bump_view_allocator_available_aligned(&allocator->view, alignment);
}
#endif
