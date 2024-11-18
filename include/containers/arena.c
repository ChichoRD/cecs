#include "arena.h"

block block_create(size_t capacity) {
    block b;
    b.size = 0;
    b.capacity = capacity;
    b.data = (uint8_t*)calloc(capacity, sizeof(uint8_t));
    b.owns_data = true;
    return b;
}

void* block_alloc(block* b, size_t size) {
    size_t remaining = b->capacity - b->size;
    assert((remaining >= size) && "Requested size exceeds block capacity");

    void* ptr = b->data + b->size;
    size_t padding = block_alignment_padding_from_size(ptr, size);
    b->size += size + padding;
    return (uint8_t*)ptr + padding;
}

void block_free(block* b) {
    b->size = 0;
    b->capacity = 0;
    uint8_t* data = b->data;
    b->data = NULL;

    if (b->owns_data)
        free(data);
}

block block_create_from_existing(size_t capacity, size_t size, uint8_t* data) {
    block b;
    b.size = size;
    b.capacity = capacity;
    b.data = data;
    b.owns_data = false;
    return b;
}

size_t block_alignment_padding_from_size(uint8_t* ptr, size_t structure_size) {
    union max_alignment {
        uintmax_t a;
        uintptr_t b;
        long double c;
    };
    uint_fast8_t alignment = (uint_fast8_t)min(structure_size, sizeof(union max_alignment));

#define ALIGNMENT_2 2
#define ALIGNMENT_4 4
#define ALIGNMENT_8 8
    switch (alignment) {
    case 0: {
        assert(false && "Alignment cannot be 0");
        return 0;
    }
    case 1: {
        return 0;
    }
    case ALIGNMENT_2:
    case 3: {
        return (ALIGNMENT_2 - ((uintptr_t)ptr & (ALIGNMENT_2 - 1))) & (ALIGNMENT_2 - 1);
    }
    case ALIGNMENT_4:
    case 5:
    case 6:
    case 7: {
        return (ALIGNMENT_4 - ((uintptr_t)ptr & (ALIGNMENT_4 - 1))) & (ALIGNMENT_4 - 1);
    }
    case ALIGNMENT_8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15: {
        return (ALIGNMENT_8 - ((uintptr_t)ptr & (ALIGNMENT_8 - 1))) & (ALIGNMENT_8 - 1);
    }
    default: {
        uint_fast16_t closest_alignment = ALIGNMENT_8;
        while (closest_alignment << 1 <= alignment) {
            closest_alignment <<= 1;
        }

        return (closest_alignment - ((uintptr_t)ptr & (closest_alignment - 1))) & (closest_alignment - 1);
    }
    }
#undef ALIGNMENT_2
#undef ALIGNMENT_4
#undef ALIGNMENT_8
}

bool block_can_alloc(const block* b, size_t structure_size) {
    return b->size + structure_size + block_alignment_padding_from_size(b->data + b->size, structure_size) <= b->capacity;
}

linked_block linked_block_create(block b, linked_block* next) {
    linked_block lb;
    lb.b = b;
    lb.next = next;
    return lb;
}

linked_block* arena_prepend(arena* a, block b) {
    linked_block* new = (linked_block*)malloc(sizeof(linked_block));
    *new = linked_block_create(b, a->first_block);

    a->first_block = new;
    if (a->last_block == NULL) {
        a->last_block = new;
    }
    return new;
}

block* arena_add_block_exact(arena* a, size_t capacity) {
    block b = block_create(capacity);
    return &arena_prepend(a, b)->b;
}

block* arena_add_block(arena* a, size_t size) {
    if (a->last_block == NULL) {
        return arena_add_block_exact(a, size > DEFAULT_BLOCK_CAPACITY ? size : DEFAULT_BLOCK_CAPACITY);
    }
    else {
        return arena_add_block_exact(a, size > a->last_block->b.capacity ? size : a->last_block->b.capacity);
    }
}

arena arena_create_with_capacity(size_t capacity) {
    arena a = arena_create();
    arena_add_block_exact(&a, capacity);
    return a;
}

arena_reallocation_strategy arena_realloc_find_fit(arena* a, void* data_block, size_t current_size, size_t new_size, linked_block** out_old_data_block, linked_block** out_fit) {
    *out_fit = NULL;
    linked_block* old_data_block = NULL;
    linked_block* current = a->first_block;
    arena_reallocation_strategy strategy = arena_reallocate_none;
    while (
        current != NULL
        && (strategy == arena_reallocate_none || old_data_block == NULL)
        ) {
        bool in_data_block = ((uint8_t*)data_block >= current->b.data)
            && ((uint8_t*)data_block + current_size <= (current->b.data + current->b.size));
        if (in_data_block) {
            assert(old_data_block == NULL && "error: data cannot be present simultaneously in multiple blocks");
            old_data_block = current;

            bool data_is_last_in_block = (uint8_t*)data_block + current_size == current->b.data + current->b.size;
            if (
                data_is_last_in_block
                && (new_size <= current_size || block_can_alloc(&current->b, new_size - current_size))
                ) {
                strategy = arena_reallocate_in_place;
            }
        }

        if (block_can_alloc(&current->b, new_size) && strategy == arena_reallocate_none) {
            *out_fit = current;

            if (current_size >= current->b.capacity / 4) {
                strategy = arena_reallocate_split;
            }
            else {
                strategy = arena_reallocate_fit;
            }
        }
        current = current->next;
    }
    if (current == NULL && strategy == arena_reallocate_none) {
        strategy = arena_reallocate_new;
    }

    *out_old_data_block = old_data_block;
    return strategy;
}

bool arena_split_block_at(void* split, linked_block* block, linked_block** out_split_block) {
    assert(block != NULL && "error: block must not be NULL");
    ptrdiff_t split_bytes = (uint8_t*)split - block->b.data;

    assert(
        split_bytes >= 0
        && split_bytes <= (ptrdiff_t)block->b.size
        && "error: data block was not allocated in this block"
    );
    size_t split_block_capacity = block->b.capacity - split_bytes;
    size_t split_block_size = block->b.size - split_bytes;

    if (split_block_capacity == 0) {
        *out_split_block = NULL;
        return false;
    }
    else {
        linked_block* split_block = malloc(sizeof(linked_block));
        *split_block = linked_block_create(
            block_create_from_existing(split_block_capacity, split_block_size, block->b.data + split_bytes),
            block->next
        );

        block->next = split_block;
        block->b.capacity = split_bytes;

        *out_split_block = split_block;
        return true;
    }
}

void* arena_realloc(arena* a, void* data_block, size_t current_size, size_t new_size) {
    size_t transfer_size = current_size < new_size ? current_size : new_size;

    if (data_block == NULL || current_size == 0) {
        return arena_alloc(a, new_size);
    }

    linked_block* old_data_block;
    linked_block* fit;
    switch (arena_realloc_find_fit(a, data_block, current_size, new_size, &old_data_block, &fit)) {
    case arena_reallocate_in_place: {
        assert(old_data_block != NULL && "error: no data block found in arena");
        old_data_block->b.size += (ptrdiff_t)new_size - (ptrdiff_t)current_size;
        assert(old_data_block->b.size <= old_data_block->b.capacity && "error: reallocation exceeds block capacity");
        return data_block;
    }
    case arena_reallocate_split: {
        assert(old_data_block != NULL && "error: no data block found in arena");
        assert(fit != NULL && "error: no fit block found in arena");

        linked_block* split_block = NULL;
        if (arena_split_block_at((uint8_t*)data_block + current_size, old_data_block, &split_block)) {
            if (old_data_block == a->last_block) {
                a->last_block = split_block;
            }
        }
        else {
            assert(split_block == NULL && "error: an split block should not have been created if there was no capacity");
        }
        old_data_block->b.size = old_data_block->b.capacity - current_size;

        uint8_t* new_data_block = NULL;
        if (fit == old_data_block && block_can_alloc(&old_data_block->b, new_size)) {
            new_data_block = block_alloc(&old_data_block->b, new_size);
        }
        else if (fit == old_data_block && split_block != NULL && block_can_alloc(&split_block->b, new_size)) {
            new_data_block = block_alloc(&split_block->b, new_size);
        }
        else {
            new_data_block = block_alloc(&fit->b, new_size);
        }

        if (new_data_block != data_block)
            memcpy(new_data_block, data_block, transfer_size);
        return new_data_block;
    }
    case arena_reallocate_fit: {
        assert(fit != NULL && "error: no fit block found in arena");
        uint8_t* new_data_block = block_alloc(&fit->b, new_size);
        if (new_data_block != data_block)
            memcpy(new_data_block, data_block, transfer_size);
        return new_data_block;
    }
    case arena_reallocate_new: {
        uint8_t* new_data_block = block_alloc(arena_add_block(a, new_size), new_size);
        memcpy(new_data_block, data_block, transfer_size);
        return new_data_block;
    }
    default: {
        assert(false && "unreachable: invalid arena reallocation strategy");
        return NULL;
    }
    }
}

arena_dbg_info arena_get_dbg_info_compare_capacity(const arena* a) {
    arena_dbg_info info = { 0 };
    info.smallest_block_capacity = SIZE_MAX;
    info.smallest_block_size = SIZE_MAX;

    linked_block* current = a->first_block;
    while (current != NULL) {
        ++info.block_count;
        if (current->b.owns_data) {
            ++info.owned_block_count;
        }
        info.total_capacity += current->b.capacity;
        info.total_size += current->b.size;

        if (current->b.capacity > info.largest_block_capacity) {
            info.largest_block_capacity = current->b.capacity;
            info.largest_block_size = current->b.size;
        }
        if (current->b.capacity < info.smallest_block_capacity) {
            info.smallest_block_capacity = current->b.capacity;
            info.smallest_block_size = current->b.size;
        }

        if (current->b.capacity - current->b.size > info.largest_remaining_capacity) {
            info.largest_remaining_capacity = current->b.capacity - current->b.size;
        }

        current = current->next;
    }
    return info;
}

arena_dbg_info arena_get_dbg_info_compare_size(const arena* a) {
    arena_dbg_info info = { 0 };
    info.smallest_block_capacity = SIZE_MAX;
    info.smallest_block_size = SIZE_MAX;
    linked_block* current = a->first_block;
    while (current != NULL) {
        ++info.block_count;
        if (current->b.owns_data) {
            ++info.owned_block_count;
        }
        info.total_capacity += current->b.capacity;
        info.total_size += current->b.size;

        if (current->b.size > info.largest_block_size) {
            info.largest_block_capacity = current->b.capacity;
            info.largest_block_size = current->b.size;
        }

        if (current->b.size < info.smallest_block_size) {
            info.smallest_block_capacity = current->b.capacity;
            info.smallest_block_size = current->b.size;
        }

        if (current->b.capacity - current->b.size > info.largest_remaining_capacity) {
            info.largest_remaining_capacity = current->b.capacity - current->b.size;
        }

        current = current->next;
    }
    return info;
}

arena_dbg_info arena_get_dbg_info_pick_smallest(const arena* a) {
    arena_dbg_info info = { 0 };
    info.smallest_block_capacity = SIZE_MAX;
    info.smallest_block_size = SIZE_MAX;

    linked_block* current = a->first_block;
    while (current != NULL) {
        ++info.block_count;
        if (current->b.owns_data) {
            ++info.owned_block_count;
        }
        info.total_capacity += current->b.capacity;
        info.total_size += current->b.size;

        if (current->b.capacity < info.smallest_block_capacity) {
            info.smallest_block_capacity = current->b.capacity;
        }
        if (current->b.size < info.smallest_block_size) {
            info.smallest_block_size = current->b.size;
        }

        if (current->b.capacity > info.largest_block_capacity) {
            info.largest_block_capacity = current->b.capacity;
        }
        if (current->b.size > info.largest_block_size) {
            info.largest_block_size = current->b.size;
        }

        if (current->b.capacity - current->b.size > info.largest_remaining_capacity) {
            info.largest_remaining_capacity = current->b.capacity - current->b.size;
        }

        current = current->next;
    }
    return info;
}

void arena_free(arena* a) {
    linked_block* current = a->first_block;
    while (current != NULL) {
        linked_block* next = current->next;
        if (current == NULL) assert(false && "current must not be NULL");
        linked_block_free(current);
        if (current == NULL) assert(false && "current must not be NULL");
        free(current);
        current = next;
    }

    a->first_block = NULL;
    a->last_block = NULL;
}

void* arena_alloc(arena* a, size_t size) {
    linked_block* current = a->first_block;
    while (current != NULL) {
        if (block_can_alloc(&current->b, size)) {
            return block_alloc(&current->b, size);
        }
        current = current->next;
    }

    return block_alloc(arena_add_block(a, size), size);
}

arena arena_create(void) {
    arena a;
    a.first_block = NULL;
    a.last_block = NULL;
    return a;
}

void linked_block_free(linked_block* lb) {
    block_free(&lb->b);
    lb->b = (block){ 0 };
    lb->next = NULL;
}
