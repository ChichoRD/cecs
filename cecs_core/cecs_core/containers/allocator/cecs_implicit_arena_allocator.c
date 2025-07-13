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
static inline void cecs_implicit_arena_allocator_network_sort(
    cecs_implicit_arena_allocator *allocator
) {
    static_assert(
        CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT == 6,
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
cecs_implicit_arena_allocator_node cecs_implicit_arena_allocator_alloc_from_free_node(
    const cecs_implicit_arena_allocator_node *previous_node, uint8_t *const block_start, const size_t size
) {
    assert(
        size <= previous_node->next_size && "fatal error: size is greater than the next_size of the node"
    );
    assert(
        block_start + previous_node->next_size <= cecs_implicit_arena_allocator_node_end(*previous_node)
        && "fatal error: block_start is out of bounds. Not enough space in the node"
    );
    const uint8_t *const allocation_end = block_start + size;
    const uint8_t *const node_end = cecs_implicit_arena_allocator_node_end(*previous_node);
    assert(
        allocation_end <= node_end
        && "fatal error: allocation_end is out of bounds. Not enough space in the node"
    );
    const size_t remaining_size = node_end - allocation_end;
    if (remaining_size >= sizeof(cecs_implicit_arena_allocator_node)) {
        return (cecs_implicit_arena_allocator_node){
            .next = previous_node->next,
            .next_size = remaining_size
        };
        static_assert(false, "TODO: case where remaining size is less than next size");
    } else {
        return (cecs_implicit_arena_allocator_node){
            .next = previous_node->next->next,
            .next_size = previous_node->next->next_size
        };
    }
}

extern void *restrict cecs_arena_allocator_alloc_aligned_advance(cecs_arena_allocator *allocator, const size_t size, const size_t alignment);

void *cecs_implicit_arena_allocator_alloc_aligned(cecs_implicit_arena_allocator *allocator, const size_t size, const size_t alignment) {
    cecs_bump_view_allocator *const current_bump = cecs_arena_allocator_current_bump(&allocator->arena);
    if (cecs_bump_view_allocator_available_aligned(current_bump, alignment) < (ptrdiff_t)size) {
        cecs_implicit_arena_allocator_node *const largest_free_block = &allocator->largest_free_blocks[0];
        const uint8_t *const block_start = cecs_implicit_arena_allocator_node_start(*largest_free_block);
        const uint8_t *const aligned_block_start = cecs_aligned_ptr(block_start, alignment);
        if (aligned_block_start + size <= cecs_implicit_arena_allocator_node_end(*largest_free_block)) {
            *largest_free_block = cecs_implicit_arena_allocator_alloc_from_free_node(
                largest_free_block, (uint8_t *)aligned_block_start, size
            );
            cecs_implicit_arena_allocator_network_sort(allocator);
            return aligned_block_start;
        } else {
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
        new_smallest_free_block->next = allocator->smallest_free_block.next->next;
        new_smallest_free_block->next_size = allocator->smallest_free_block.next->next_size;

        allocator->smallest_free_block.next->next = new_smallest_free_block;
        allocator->smallest_free_block.next->next_size = block_size;

        allocator->smallest_free_block = *new_smallest_free_block;
        static_assert(false, "TODO: think correct smallest replacement and filling the largests and replacing the heads");
    } else {
        size_t i = 0;
        while (
            (i < CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT)
            && (block_size < allocator->largest_free_blocks[i].next_size)
        ) {
            ++i;
        }
        cecs_implicit_arena_allocator_node *const new_free_block =
            (uint8_t *)block + block_size - sizeof(cecs_implicit_arena_allocator_node);
        if (i < CECS_IMPLICIT_ARENA_ALLOCATOR_FREE_LISTS_COUNT) {
            new_free_block->next = allocator->largest_free_blocks[i].next;
            new_free_block->next_size = allocator->largest_free_blocks[i].next_size;

            allocator->largest_free_blocks[i].next = new_free_block;
            allocator->largest_free_blocks[i].next_size = block_size;

            cecs_implicit_arena_allocator_network_sort(allocator);
        } else {
            new_free_block->next = allocator->smallest_free_block.next->next;
            new_free_block->next_size = allocator->smallest_free_block.next->next_size;

            allocator->smallest_free_block.next->next = new_free_block;
            allocator->smallest_free_block.next->next_size = block_size;
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
