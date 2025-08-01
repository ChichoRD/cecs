#include "cecs_implicit_arena_allocator.h"
#include <memory.h>

static inline void cecs_implicit_arena_allocator_node_swap_lt(
    cecs_implicit_arena_allocator_node *a,
    cecs_implicit_arena_allocator_node *b
) {
    if (a->next_size < b->next_size) {
        cecs_implicit_arena_allocator_node temp = *a;
        *a = *b;
        *b = temp;
    }
}
static inline void cecs_implicit_arena_allocator_node_network_sort_6(
    cecs_implicit_arena_allocator_node nodes[6]
) {
    cecs_implicit_arena_allocator_node_swap_lt(&nodes[0], &nodes[5]);
    cecs_implicit_arena_allocator_node_swap_lt(&nodes[1], &nodes[3]);
    cecs_implicit_arena_allocator_node_swap_lt(&nodes[2], &nodes[4]);

    cecs_implicit_arena_allocator_node_swap_lt(&nodes[0], &nodes[1]);
    cecs_implicit_arena_allocator_node_swap_lt(&nodes[2], &nodes[3]);

    cecs_implicit_arena_allocator_node_swap_lt(&nodes[0], &nodes[3]);
    cecs_implicit_arena_allocator_node_swap_lt(&nodes[2], &nodes[5]);

    cecs_implicit_arena_allocator_node_swap_lt(&nodes[0], &nodes[1]);
    cecs_implicit_arena_allocator_node_swap_lt(&nodes[2], &nodes[3]);
    cecs_implicit_arena_allocator_node_swap_lt(&nodes[4], &nodes[5]);

    cecs_implicit_arena_allocator_node_swap_lt(&nodes[1], &nodes[2]);
    cecs_implicit_arena_allocator_node_swap_lt(&nodes[3], &nodes[4]);
}

static const size_t cecs_implicit_arena_allocator_free_lists_count = CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT;
static inline void cecs_implicit_arena_allocator_network_sort(
    cecs_implicit_arena_allocator *allocator
) {
    static_assert(
        cecs_implicit_arena_allocator_free_lists_count == 6,
        "fatal static error: cecs_implicit_arena_allocator_network_sort supports exactly 6 free lists"
    );
    cecs_implicit_arena_allocator_node_network_sort_6(allocator->largest_free_blocks);
}

static inline uint8_t *cecs_implicit_arena_allocator_node_next_start(const cecs_implicit_arena_allocator_node node) {
    return (uint8_t *)node.next + sizeof(node) - node.next_size;
}
static inline uint8_t *cecs_implicit_arena_allocator_node_next_end(const cecs_implicit_arena_allocator_node node) {
    return (uint8_t *)node.next + sizeof(node);
}


extern void *cecs_arena_allocator_alloc_aligned_advance(cecs_arena_allocator *allocator, const size_t size, const size_t alignment);
extern inline cecs_bump_allocator *cecs_arena_allocator_current_bump(cecs_arena_allocator *allocator);

static inline cecs_implicit_arena_allocator_node *cecs_implicit_arena_allocator_append_generic(
    cecs_implicit_arena_allocator *allocator, void *const new_free_block, size_t block_size
) {
    if (allocator->generic_free_block.next == NULL) {
        assert(false && "fatal error: generic free block is NULL");
        exit(EXIT_FAILURE);
    }
    *(cecs_implicit_arena_allocator_node *)new_free_block = (cecs_implicit_arena_allocator_node){
        .next = allocator->generic_free_block.next->next,
        .next_size = allocator->generic_free_block.next->next_size
    };
    *allocator->generic_free_block.next = (cecs_implicit_arena_allocator_node){
        .next = (cecs_implicit_arena_allocator_node *)new_free_block,
        .next_size = block_size
    };
    return &allocator->generic_free_block;
}
static inline cecs_implicit_arena_allocator_node *cecs_implicit_arena_allocator_prepend_generic(
    cecs_implicit_arena_allocator *allocator, void *const new_free_block, size_t block_size
) {
    *(cecs_implicit_arena_allocator_node *)new_free_block = (cecs_implicit_arena_allocator_node){
        .next = allocator->generic_free_block.next,
        .next_size = allocator->generic_free_block.next_size
    };
    allocator->generic_free_block = (cecs_implicit_arena_allocator_node){
        .next = new_free_block,
        .next_size = block_size
    };
    return &allocator->generic_free_block;
}
static inline cecs_implicit_arena_allocator_node *cecs_implicit_arena_allocator_prepend_larger(
    cecs_implicit_arena_allocator *const allocator, void *const new_free_block, const size_t block_size, const size_t largest_index
) {
    if (largest_index >= cecs_implicit_arena_allocator_free_lists_count) {
        assert(false && "fatal error: largest_index is out of bounds");
        exit(EXIT_FAILURE);
    }
    if (block_size < allocator->largest_free_blocks[largest_index].next_size) {
        assert(false && "fatal error: block_size is less than the next_size of the largest free block");
        exit(EXIT_FAILURE);
    }
    
    *(cecs_implicit_arena_allocator_node *)new_free_block = (cecs_implicit_arena_allocator_node){
        .next = allocator->largest_free_blocks[largest_index].next,
        .next_size = allocator->largest_free_blocks[largest_index].next_size,
    };
    allocator->largest_free_blocks[largest_index] = (cecs_implicit_arena_allocator_node){
        .next = new_free_block,
        .next_size = block_size,
    };

    cecs_implicit_arena_allocator_network_sort(allocator);
    return &allocator->largest_free_blocks[largest_index];
}
static inline size_t cecs_implicit_arena_allocator_find_maximum_index(
    const cecs_implicit_arena_allocator *allocator, const size_t block_size, const size_t start_index
) {
    size_t i = start_index;
    while (
        (i < CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT)
        && (block_size < allocator->largest_free_blocks[i].next_size)
    ) {
        ++i;
    }
    return i;
}

static void *cecs_implicit_arena_allocator_alloc_aligned_from_largest(
    cecs_implicit_arena_allocator *allocator,
    cecs_implicit_arena_allocator_node *const largest_free_block,
    uint8_t *const aligned_block_start,
    const size_t size
){
    if (largest_free_block->next_size < allocator->largest_free_blocks[0].next_size) {
        assert(
            false
            && "fatal error: largest free block is smaller than the first free block."
            "The list is unsorted or the wrong block was selected."
        );
        exit(EXIT_FAILURE);
    }
    if (largest_free_block->next_size < size) {
        assert(false && "fatal error: largest free block size is less than requested size");
        exit(EXIT_FAILURE);
    }
    if (aligned_block_start < cecs_implicit_arena_allocator_node_next_start(*largest_free_block)) {
        assert(false && "fatal error: aligned block start is before the start of the largest free block");
        exit(EXIT_FAILURE);
    }
    if (aligned_block_start + size > cecs_implicit_arena_allocator_node_next_end(*largest_free_block)) {
        assert(false && "fatal error: aligned block end is after the end of the largest free block");
        exit(EXIT_FAILURE);
    }

    const uint8_t *const allocation_end = aligned_block_start + size;
    const size_t remaining_size = cecs_implicit_arena_allocator_node_next_end(*largest_free_block) - allocation_end;
    if (remaining_size < sizeof(cecs_implicit_arena_allocator_node)) {
        *largest_free_block = *largest_free_block->next;
    } else if (remaining_size < largest_free_block->next->next_size) {
        *largest_free_block = *largest_free_block->next;
        const size_t next_maximum = cecs_implicit_arena_allocator_find_maximum_index(allocator, remaining_size, 1);
        if (next_maximum < cecs_implicit_arena_allocator_free_lists_count) {
            cecs_implicit_arena_allocator_prepend_larger(
                allocator, (cecs_implicit_arena_allocator_node *)allocation_end, remaining_size, next_maximum
            );
        } else if (remaining_size >= allocator->generic_free_block.next_size) {
            cecs_implicit_arena_allocator_prepend_generic(
                allocator, (cecs_implicit_arena_allocator_node *)allocation_end, remaining_size
            );
        } else {
            cecs_implicit_arena_allocator_append_generic(
                allocator, (cecs_implicit_arena_allocator_node *)allocation_end, remaining_size
            );
        }
    } else {
        largest_free_block->next_size = remaining_size;
    }

    cecs_implicit_arena_allocator_network_sort(allocator);
    return aligned_block_start;
}
static void *cecs_implicit_arena_allocator_alloc_aligned_from_generic(
    cecs_implicit_arena_allocator *allocator,
    uint8_t *const aligned_block_start,
    const size_t size
) {
    if (allocator->generic_free_block.next == NULL) {
        assert(false && "fatal error: generic free block is NULL");
        exit(EXIT_FAILURE);
    }
    if (aligned_block_start < cecs_implicit_arena_allocator_node_next_start(allocator->generic_free_block)) {
        assert(false && "fatal error: aligned block start is before the start of the generic free block");
        exit(EXIT_FAILURE);
    }
    if (aligned_block_start + size > cecs_implicit_arena_allocator_node_next_end(allocator->generic_free_block)) {
        assert(false && "fatal error: aligned block end is after the end of the generic free block");
        exit(EXIT_FAILURE);
    }

    const uint8_t *const allocation_end = aligned_block_start + size;
    const size_t remaining_size = cecs_implicit_arena_allocator_node_next_end(allocator->generic_free_block) - allocation_end;
    if (remaining_size < sizeof(cecs_implicit_arena_allocator_node)) {
        allocator->generic_free_block = (cecs_implicit_arena_allocator_node){
            .next = allocator->generic_free_block.next->next,
            .next_size = allocator->generic_free_block.next->next_size
        };
    } else if (remaining_size < allocator->generic_free_block.next->next_size) {
        // traverse the the list to find the first node that points to a smaller block than the remaining size
        cecs_implicit_arena_allocator_node *current = &allocator->generic_free_block.next;
        do {
            current = current->next;
            assert(current != NULL && "fatal error: reached end of generic free block list without finding a suitable node");
        } while (remaining_size < current->next_size);
        *(cecs_implicit_arena_allocator_node *)allocation_end = (cecs_implicit_arena_allocator_node){
            .next = current->next,
            .next_size = current->next_size
        };
        *current = (cecs_implicit_arena_allocator_node){
            .next = (cecs_implicit_arena_allocator_node *)allocation_end,
            .next_size = remaining_size
        };
        allocator->generic_free_block = *allocator->generic_free_block.next;
    } else {
        allocator->generic_free_block.next_size = remaining_size;
    }

    return aligned_block_start;
}
void *cecs_implicit_arena_allocator_alloc_aligned(cecs_implicit_arena_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_bump_allocator *const current_bump = cecs_arena_allocator_current_bump(&allocator->arena);
    cecs_implicit_arena_allocator_node *const largest_free_block = &allocator->largest_free_blocks[0];

    uint8_t *const largest_block_start = cecs_implicit_arena_allocator_node_next_start(*largest_free_block);
    uint8_t *const aligned_largest_block_start = cecs_aligned_ptr_mut(largest_block_start, alignment);

    uint8_t *const generic_block_start = cecs_implicit_arena_allocator_node_next_start(allocator->generic_free_block);
    uint8_t *const aligned_generic_block_start = cecs_aligned_ptr_mut(generic_block_start, alignment);
    if (aligned_largest_block_start + size <= cecs_implicit_arena_allocator_node_next_end(*largest_free_block)) {
        return cecs_implicit_arena_allocator_alloc_aligned_from_largest(allocator, largest_free_block, aligned_largest_block_start, size);
    } else if (aligned_generic_block_start + size <= cecs_implicit_arena_allocator_node_next_end(allocator->generic_free_block)) {
        return cecs_implicit_arena_allocator_alloc_aligned_from_generic(allocator, aligned_generic_block_start, size);
    } else if (cecs_bump_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)size) {
        cecs_implicit_arena_allocator_free(allocator, current_bump->view.next, cecs_bump_allocator_available(current_bump));
        return cecs_arena_allocator_alloc_aligned_advance(&allocator->arena, size, alignment);
    } else {
        return cecs_bump_allocator_alloc_aligned_expect(current_bump, size, alignment);
    }
}

void *cecs_implicit_arena_allocator_alloc(cecs_implicit_arena_allocator *allocator, const size_t size) {
    return cecs_implicit_arena_allocator_alloc_aligned(allocator, size, cecs_max_alignment_from_size(size));
}

void cecs_implicit_arena_allocator_free(cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size) {
    assert(block != NULL && "fatal error: block is NULL");
    if (block_size < sizeof(cecs_implicit_arena_allocator_node)) {
        return;
    }

    uint8_t *const new_free_block =
        (uint8_t *)block + block_size - sizeof(cecs_implicit_arena_allocator_node);

    const size_t largest_index = cecs_implicit_arena_allocator_find_maximum_index(allocator, block_size, 0);
    if (largest_index < cecs_implicit_arena_allocator_free_lists_count) {
        if (cecs_implicit_arena_allocator_node_next_end(allocator->largest_free_blocks[largest_index]) == (uint8_t *)block) {
            *(cecs_implicit_arena_allocator_node *)new_free_block = (cecs_implicit_arena_allocator_node){
                .next = allocator->largest_free_blocks[largest_index].next->next,
                .next_size = allocator->largest_free_blocks[largest_index].next->next_size
            };
            allocator->largest_free_blocks[largest_index] = (cecs_implicit_arena_allocator_node){
                .next = (cecs_implicit_arena_allocator_node *)new_free_block,
                .next_size = allocator->largest_free_blocks[largest_index].next_size + block_size
            };
            cecs_implicit_arena_allocator_network_sort(allocator);
        } else {
            cecs_implicit_arena_allocator_prepend_larger(
                allocator, new_free_block, block_size, largest_index
            );
        }
    } else if (block_size >= allocator->generic_free_block.next_size) {
        cecs_implicit_arena_allocator_prepend_generic(
            allocator, new_free_block, block_size
        );
    } else {
        cecs_implicit_arena_allocator_append_generic(
            allocator, new_free_block, block_size
        );
    }
}

void *cecs_implicit_arena_allocator_realloc_aligned(
    cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    void *new_block;
    if (new_size < block_size) {
        new_block = block;
        cecs_implicit_arena_allocator_free(allocator, (uint8_t *)block + new_size, block_size - new_size);
    } else if (new_size > block_size) {
        cecs_bump_allocator *const current_bump = cecs_arena_allocator_current_bump(&allocator->arena);
        if (
            ((uint8_t *)block + block_size == current_bump->view.next)
            && (cecs_bump_allocator_available_aligned(current_bump, alignment) >= (ptrdiff_t)(new_size - block_size))
        ) {
            new_block = cecs_bump_allocator_realloc_aligned_expect(
                current_bump, block, block_size, new_size, alignment
            );
        } else {
            new_block = cecs_implicit_arena_allocator_alloc_aligned(allocator, new_size, alignment);
            memcpy(new_block, block, block_size);
            cecs_implicit_arena_allocator_free(allocator, block, block_size);
        }
    } else {
        new_block = block;
    }
    return new_block;
}
void *cecs_implicit_arena_allocator_realloc(
    cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size
) {
    return cecs_implicit_arena_allocator_realloc_aligned(
        allocator, block, block_size, new_size, cecs_max_alignment_from_size(new_size)
    );
}

void cecs_implicit_arena_allocator_reset(cecs_implicit_arena_allocator *allocator){
    cecs_arena_allocator_reset(&allocator->arena);
    allocator->generic_free_block = (cecs_implicit_arena_allocator_node){
        .next = NULL,
        .next_size = 0
    };
    for (size_t i = 0; i < CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT; ++i) {
        allocator->largest_free_blocks[i] = (cecs_implicit_arena_allocator_node){0};
    }
}
void cecs_implicit_arena_allocator_destroy(cecs_implicit_arena_allocator *allocator) {
    cecs_arena_allocator_destroy(&allocator->arena);
    allocator->generic_free_block = (cecs_implicit_arena_allocator_node){
        .next = NULL,
        .next_size = 0
    };
    for (size_t i = 0; i < CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT; ++i) {
        allocator->largest_free_blocks[i] = (cecs_implicit_arena_allocator_node){0};
    }
}
