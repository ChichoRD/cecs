#include "cecs_implicit_arena_allocator.h"

static inline void cecs_implicit_arena_allocator_node_swap_ge(
    cecs_implicit_arena_allocator_node *a,
    cecs_implicit_arena_allocator_node *b
) {
    if (a->next_size > b->next_size) {
        cecs_implicit_arena_allocator_node temp = *a;
        *a = *b;
        *b = temp;
    }
}
static inline void cecs_implicit_arena_allocator_node_network_sort_6(
    cecs_implicit_arena_allocator_node nodes[6]
) {
    cecs_implicit_arena_allocator_node_swap_ge(&nodes[0], &nodes[5]);
    cecs_implicit_arena_allocator_node_swap_ge(&nodes[1], &nodes[3]);
    cecs_implicit_arena_allocator_node_swap_ge(&nodes[2], &nodes[4]);

    cecs_implicit_arena_allocator_node_swap_ge(&nodes[0], &nodes[1]);
    cecs_implicit_arena_allocator_node_swap_ge(&nodes[2], &nodes[3]);

    cecs_implicit_arena_allocator_node_swap_ge(&nodes[0], &nodes[3]);
    cecs_implicit_arena_allocator_node_swap_ge(&nodes[2], &nodes[5]);

    cecs_implicit_arena_allocator_node_swap_ge(&nodes[0], &nodes[1]);
    cecs_implicit_arena_allocator_node_swap_ge(&nodes[2], &nodes[3]);
    cecs_implicit_arena_allocator_node_swap_ge(&nodes[4], &nodes[5]);

    cecs_implicit_arena_allocator_node_swap_ge(&nodes[1], &nodes[2]);
    cecs_implicit_arena_allocator_node_swap_ge(&nodes[3], &nodes[4]);
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

static inline const uint8_t *cecs_implicit_arena_allocator_node_start(const cecs_implicit_arena_allocator_node node) {
    return (uint8_t *)node.next + sizeof(node) - node.next_size;
}
static inline const uint8_t *cecs_implicit_arena_allocator_node_end(const cecs_implicit_arena_allocator_node node) {
    return (uint8_t *)node.next + sizeof(node);
}


extern void *restrict cecs_arena_allocator_alloc_aligned_advance(cecs_arena_allocator *allocator, const size_t size, const size_t alignment);
extern inline cecs_bump_view_allocator *cecs_arena_allocator_current_bump(cecs_arena_allocator *allocator);

static inline cecs_implicit_arena_allocator_node *cecs_implicit_arena_allocator_append_smallest(
    cecs_implicit_arena_allocator *allocator, void *const new_free_block, size_t block_size
) {
    *(cecs_implicit_arena_allocator_node *)new_free_block = (cecs_implicit_arena_allocator_node){
        .next = allocator->smallest_free_block.next,
        .next_size = allocator->smallest_free_block.next_size
    };

    allocator->smallest_free_block.next = new_free_block;
    allocator->smallest_free_block.next_size = block_size;
    return &allocator->smallest_free_block;
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
    allocator->largest_free_blocks[largest_index].next = new_free_block;
    allocator->largest_free_blocks[largest_index].next_size = block_size;

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

void *cecs_implicit_arena_allocator_alloc_aligned(cecs_implicit_arena_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_bump_view_allocator *const current_bump = cecs_arena_allocator_current_bump(&allocator->arena);
    if (cecs_bump_view_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)size) {
        cecs_implicit_arena_allocator_node *const largest_free_block = &allocator->largest_free_blocks[0];
        const uint8_t *const block_start = cecs_implicit_arena_allocator_node_start(*largest_free_block);
        const uint8_t *const aligned_block_start = cecs_aligned_ptr(block_start, alignment);
        if (aligned_block_start + size <= cecs_implicit_arena_allocator_node_end(*largest_free_block)) {
            assert(largest_free_block->next_size >= size && "fatal error: largest free block size is less than requested size");

            const uint8_t *const allocation_end = aligned_block_start + size;
            const size_t remaining_size = cecs_implicit_arena_allocator_node_end(*largest_free_block) - allocation_end;
            if (remaining_size < sizeof(cecs_implicit_arena_allocator_node)) {
                *largest_free_block = *largest_free_block->next;
            } else if (remaining_size < largest_free_block->next->next_size) {
                const size_t next_maximum = cecs_implicit_arena_allocator_find_maximum_index(allocator, remaining_size, 1);
                if (next_maximum < cecs_implicit_arena_allocator_free_lists_count) {
                    cecs_implicit_arena_allocator_prepend_larger(
                        allocator, (cecs_implicit_arena_allocator_node *)allocation_end, remaining_size, next_maximum
                    );
                } else {
                    cecs_implicit_arena_allocator_append_smallest(
                        allocator, (cecs_implicit_arena_allocator_node *)allocation_end, remaining_size
                    );
                }
            } else {
                largest_free_block->next_size = remaining_size;
            }

            cecs_implicit_arena_allocator_network_sort(allocator);
            return aligned_block_start;
        } else {
            cecs_bump_view_allocator *const current_bump = cecs_arena_allocator_current_bump(&allocator->arena);
            cecs_implicit_arena_allocator_free(allocator, current_bump->next, cecs_bump_view_allocator_available(current_bump));
            return cecs_arena_allocator_alloc_aligned_advance(allocator, size, alignment);
        }
    } else {
        return cecs_bump_view_allocator_alloc_aligned_expect(current_bump, size, alignment);
    }
}

void *cecs_implicit_arena_allocator_alloc(cecs_implicit_arena_allocator *allocator, const size_t size) {
    return cecs_implicit_arena_allocator_alloc_aligned(allocator, size, cecs_max_alignment_from_size(size));
}

void cecs_implicit_arena_allocator_free(cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size) {
    if (block_size < sizeof(cecs_implicit_arena_allocator_node)) {
        return;
    }

    if (block_size <= allocator->smallest_free_block.next_size) {
        cecs_implicit_arena_allocator_node *const new_smallest_free_block =
            (uint8_t *)block + block_size - sizeof(cecs_implicit_arena_allocator_node);
        cecs_implicit_arena_allocator_append_smallest(
            allocator, new_smallest_free_block, block_size
        );
    } else {
        cecs_implicit_arena_allocator_node *const new_free_block =
            (uint8_t *)block + block_size - sizeof(cecs_implicit_arena_allocator_node);
        const size_t largest_index = cecs_implicit_arena_allocator_find_maximum_index(allocator, block_size, 0);
        if (largest_index < cecs_implicit_arena_allocator_free_lists_count) {
            cecs_implicit_arena_allocator_prepend_larger(
                allocator, new_free_block, block_size, largest_index
            );
        } else {
            // If no suitable free list is found, we can just append to the smallest free block
            cecs_implicit_arena_allocator_append_smallest(
                allocator, new_free_block, block_size
            );
        }
    }
}

void *cecs_implicit_arena_allocator_realloc_aligned(
    cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size, const size_t alignment
) {
    return nullptr;
}

void *cecs_implicit_arena_allocator_realloc(
    cecs_implicit_arena_allocator *allocator, void *block, const size_t block_size, const size_t new_size
) {
    return nullptr;
}



void cecs_implicit_arena_allocator_reset(cecs_implicit_arena_allocator *allocator)
{
}
