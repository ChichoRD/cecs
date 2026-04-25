#include "cecs_allocator.h"

static inline cecs_allocator cecs_allocator_create_from(
    const cecs_internal_allocator allocator,
    const cecs_internal_allocator_type type
) {
    return (cecs_allocator){
        .allocator = allocator,
        .type = type
    };
}

cecs_allocator cecs_allocator_create_bump_alloc(const size_t size) {
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .bump = cecs_bump_allocator_create_alloc(size) },
        cecs_internal_allocator_type_bump
    );
}
cecs_allocator cecs_allocator_create_bump_virtual(const size_t page_count) {
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .bump = cecs_bump_allocator_create_virtual(page_count) },
        cecs_internal_allocator_type_bump
    );
}
cecs_allocator cecs_allocator_create_arena(size_t bump_size, size_t bump_capacity) {
    // static_assert(
    //     sizeof(size_t) <= sizeof(cecs_arena_allocator_bump_usize),
    //     "static error: bump_capacity argument of cecs_allocator_create_arena cannot be safely cast to cecs_arena_allocator_bump_usize"
    // );
    cecs_debugbreak_fail_unless(
        bump_capacity <= (size_t)CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX,
        "fatal error: bump_capacity argument of cecs_allocator_create_arena must be less than or equal to CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX"
    );
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .arena = cecs_arena_allocator_create(bump_size, (cecs_arena_allocator_bump_usize)bump_capacity) },
        cecs_internal_allocator_type_arena
    );
}
cecs_allocator cecs_allocator_create_implicit_arena(size_t bump_size, size_t bump_capacity) {
    // static_assert(
    //     sizeof(size_t) <= sizeof(cecs_arena_allocator_bump_usize),
    //     "static error: bump_capacity argument of cecs_allocator_create_implicit_arena cannot be safely cast to cecs_arena_allocator_bump_usize"
    // );
    cecs_debugbreak_fail_unless(
        bump_capacity <= (size_t)CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX,
        "fatal error: bump_capacity argument of cecs_allocator_create_implicit_arena must be less than or equal to CECS_ARENA_ALLOCATOR_BUMP_USIZE_TYPE_MAX"
    );
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .implicit_arena = cecs_implicit_arena_allocator_create(bump_size, (cecs_arena_allocator_bump_usize)bump_capacity) },
        cecs_internal_allocator_type_implicit_arena
    );
}

#define CECS_ALLOCATOR_TYPE_MAX cecs_internal_allocator_type_implicit_arena
void *cecs_allocator_alloc_aligned(cecs_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_debugbreak_fail_unless(allocator->type != cecs_internal_allocator_type_none, "fatal error: attempted to alloc_aligned from an allocator of type 'none'");
    cecs_debugbreak_fail_unless(allocator->type <= CECS_ALLOCATOR_TYPE_MAX, "fatal error: invalid allocator type in call to alloc_aligned");
    switch (allocator->type) {
    case cecs_internal_allocator_type_none:
        cecs_unreachable();
        return NULL;
    case cecs_internal_allocator_type_bump:
        return cecs_bump_allocator_alloc_aligned_expect(&allocator->allocator.bump, size, alignment);
    case cecs_internal_allocator_type_arena:
        return cecs_arena_allocator_alloc_aligned(&allocator->allocator.arena, size, alignment);
    case cecs_internal_allocator_type_implicit_arena:
        return cecs_implicit_arena_allocator_alloc_aligned(&allocator->allocator.implicit_arena, size, alignment);
    default:
        cecs_unreachable();
        return NULL;
    }
}

void *cecs_allocator_alloc(cecs_allocator *allocator, const size_t size) {
    cecs_debugbreak_fail_unless(allocator->type != cecs_internal_allocator_type_none, "fatal error: attempted to alloc from an allocator of type 'none'");
    cecs_debugbreak_fail_unless(allocator->type <= CECS_ALLOCATOR_TYPE_MAX, "fatal error: invalid allocator type in call to alloc");
    switch (allocator->type) {
    case cecs_internal_allocator_type_none:
        cecs_unreachable();
        return NULL;
    case cecs_internal_allocator_type_bump:
        return cecs_bump_allocator_alloc_expect(&allocator->allocator.bump, size);
    case cecs_internal_allocator_type_arena:
        return cecs_arena_allocator_alloc(&allocator->allocator.arena, size);
    case cecs_internal_allocator_type_implicit_arena:
        return cecs_implicit_arena_allocator_alloc(&allocator->allocator.implicit_arena, size);
    default:
        cecs_unreachable();
        return NULL;
    }
}

void *cecs_allocator_realloc_aligned(
    cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    cecs_debugbreak_fail_unless(allocator->type != cecs_internal_allocator_type_none, "fatal error: attempted to realloc_aligned from an allocator of type 'none'");
    cecs_debugbreak_fail_unless(allocator->type <= CECS_ALLOCATOR_TYPE_MAX, "fatal error: invalid allocator type in call to realloc_aligned");
    switch (allocator->type) {
    case cecs_internal_allocator_type_none:
        cecs_unreachable();
        return NULL;
    case cecs_internal_allocator_type_bump:
        return cecs_bump_allocator_realloc_aligned_expect(&allocator->allocator.bump, block, block_size, new_size, alignment);
    case cecs_internal_allocator_type_arena:
        return cecs_arena_allocator_realloc_aligned(&allocator->allocator.arena, block, block_size, new_size, alignment);
    case cecs_internal_allocator_type_implicit_arena:
        return cecs_implicit_arena_allocator_realloc_aligned(
            &allocator->allocator.implicit_arena, block, block_size, new_size, alignment
        );
    default:
        cecs_unreachable();
        return NULL;
    }
}

void *cecs_allocator_realloc(cecs_allocator *allocator, void *block, const size_t block_size, const size_t new_size) {
    cecs_debugbreak_fail_unless(allocator->type != cecs_internal_allocator_type_none, "fatal error: attempted to realloc from an allocator of type 'none'");
    cecs_debugbreak_fail_unless(allocator->type <= CECS_ALLOCATOR_TYPE_MAX, "fatal error: invalid allocator type in call to realloc");
    switch (allocator->type) {
    case cecs_internal_allocator_type_none:
        cecs_unreachable();
        return NULL;
    case cecs_internal_allocator_type_bump:
        return cecs_bump_allocator_realloc_expect(&allocator->allocator.bump, block, block_size, new_size);
    case cecs_internal_allocator_type_arena:
        return cecs_arena_allocator_realloc(&allocator->allocator.arena, block, block_size, new_size);
    case cecs_internal_allocator_type_implicit_arena:
        return cecs_implicit_arena_allocator_realloc(
            &allocator->allocator.implicit_arena, block, block_size, new_size
        );
    default:
        cecs_unreachable();
        return NULL;
    }
}

void cecs_allocator_free(cecs_allocator *allocator, void *block, const size_t block_size) {
    cecs_debugbreak_fail_unless(allocator->type != cecs_internal_allocator_type_none, "fatal error: attempted to free from an allocator of type 'none'");
    cecs_debugbreak_fail_unless(allocator->type <= CECS_ALLOCATOR_TYPE_MAX, "fatal error: invalid allocator type in call to free");
    switch (allocator->type) {
    case cecs_internal_allocator_type_none:
        cecs_unreachable();
        break;
    case cecs_internal_allocator_type_bump:
        cecs_bump_allocator_free(&allocator->allocator.bump, block, block_size);
        break;
    case cecs_internal_allocator_type_arena:
        cecs_arena_allocator_free(&allocator->allocator.arena, block, block_size);
        break;
    case cecs_internal_allocator_type_implicit_arena:
        cecs_implicit_arena_allocator_free(&allocator->allocator.implicit_arena, block, block_size);
        break;
    default:
        cecs_unreachable();
    }
}

void cecs_allocator_reset(cecs_allocator *allocator) {
    cecs_debugbreak_fail_unless(allocator->type != cecs_internal_allocator_type_none, "fatal error: attempted to reset an allocator of type 'none'");
    cecs_debugbreak_fail_unless(allocator->type <= CECS_ALLOCATOR_TYPE_MAX, "fatal error: invalid allocator type in call to reset");
    switch (allocator->type) {
    case cecs_internal_allocator_type_bump:
        cecs_bump_allocator_reset(&allocator->allocator.bump);
        break;
    case cecs_internal_allocator_type_arena:
        cecs_arena_allocator_reset(&allocator->allocator.arena);
        break;
    case cecs_internal_allocator_type_implicit_arena:
        cecs_implicit_arena_allocator_reset(&allocator->allocator.implicit_arena);
        break;
    default:
        cecs_unreachable();
    }
}

void cecs_allocator_destroy(cecs_allocator *allocator) {
    cecs_debugbreak_fail_unless(allocator->type != cecs_internal_allocator_type_none, "fatal error: attempted to destroy an allocator of type 'none'");
    cecs_debugbreak_fail_unless(allocator->type <= CECS_ALLOCATOR_TYPE_MAX, "fatal error: invalid allocator type in call to destroy");
    switch (allocator->type) {
    case cecs_internal_allocator_type_none:
        cecs_unreachable();
        break;
    case cecs_internal_allocator_type_bump:
        cecs_bump_allocator_destroy(&allocator->allocator.bump);
        break;
    case cecs_internal_allocator_type_arena:
        cecs_arena_allocator_destroy(&allocator->allocator.arena);
        break;
    case cecs_internal_allocator_type_implicit_arena:
        cecs_implicit_arena_allocator_destroy(&allocator->allocator.implicit_arena);
        break;
    default:
        cecs_unreachable();
    }
}

cecs_allocator cecs_allocator_alloc_bump_view_aligned(cecs_allocator *allocator, const size_t size, const size_t alignment) {
    void *const allocation = cecs_allocator_alloc_aligned(allocator, size, alignment);
    const cecs_memory_block block = {
        .memory_start = (uint8_t *)allocation,
        .memory_end = (uint8_t *)allocation + size,
        .reserved = size
    };
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .bump = cecs_bump_allocator_from_view(cecs_bump_view_allocator_create(block)) },
        cecs_internal_allocator_type_bump
    );
}
cecs_allocator cecs_allocator_alloc_bump_view(cecs_allocator *allocator, const size_t size) {
    void *const allocation = cecs_allocator_alloc(allocator, size);
    const cecs_memory_block block = {
        .memory_start = (uint8_t *)allocation,
        .memory_end = (uint8_t *)allocation + size,
        .reserved = size
    };
    return cecs_allocator_create_from(
        (cecs_internal_allocator){ .bump = cecs_bump_allocator_from_view(cecs_bump_view_allocator_create(block)) },
        cecs_internal_allocator_type_bump
    );
}

extern inline const cecs_bump_allocator *cecs_allocator_bump(const cecs_allocator *allocator);
extern inline cecs_bump_allocator *cecs_allocator_bump_mut(cecs_allocator *allocator);
extern inline const cecs_arena_allocator *cecs_allocator_arena(const cecs_allocator *allocator);
extern inline cecs_arena_allocator *cecs_allocator_arena_mut(cecs_allocator *allocator);
extern inline const cecs_implicit_arena_allocator *cecs_allocator_implicit_arena(const cecs_allocator *allocator);
extern inline cecs_implicit_arena_allocator *cecs_allocator_implicit_arena_mut(cecs_allocator *allocator);
