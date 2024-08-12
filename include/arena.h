#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define DEFAULT_BLOCK_CAPACITY (8 * 1024)

typedef struct block {
    size_t size;
    size_t capacity;
    uint8_t *data;
} block;

block block_create(size_t capacity) {
    block b;
    b.size = 0;
    b.capacity = capacity;
    b.data = (uint8_t *)calloc(capacity, sizeof(uint8_t));
    return b;
}

void *block_alloc(block *b, size_t size) {
    size_t remaining = b->capacity - b->size;
    assert((remaining >= size) && "Requested size exceeds block capacity");

    void *ptr = b->data + b->size;
    b->size += size;
    return ptr;
}

void block_free(block *b) {
    b->size = 0;
    b->capacity = 0;
    free(b->data);
    b->data = NULL;
}


typedef struct linked_block {
    block *b;
    struct linked_block *next;
} linked_block;

linked_block linked_block_create(block *b, linked_block *next) {
    linked_block lb;
    lb.b = b;
    lb.next = next;
    return lb;
}

void linked_block_free(linked_block *lb) {
    block_free(lb->b);
    free(lb->b);
    lb->b = NULL;
    lb->next = NULL;
}


typedef struct arena {
    linked_block *first_block;
    linked_block *last_block;
} arena;

#define ARENA_FOREACH(arena0, linked_block0) for (linked_block *(linked_block0) = (arena0).first_block; (linked_block0) != NULL; (linked_block0) = linked_block0->next)

arena arena_create(void) {
    arena a;
    a.first_block = NULL;
    a.last_block = NULL;
    return a;
}

static linked_block *arena_prepend(arena *a, block *b) {
    linked_block *new = (linked_block *)malloc(sizeof(linked_block));
    *new = linked_block_create(b, a->first_block);

    a->first_block = new;
    if (a->last_block == NULL) {
        a->last_block = new;
    }
    return new;
}

static block *arena_add_block_exact(arena *a, size_t capacity) {
    block *b = (block *)malloc(sizeof(block));
    *b = block_create(capacity);
    return arena_prepend(a, b)->b;
}

static block *arena_add_block(arena *a, size_t size) {
    return arena_add_block_exact(a, size > DEFAULT_BLOCK_CAPACITY ? size : DEFAULT_BLOCK_CAPACITY);
}

arena arena_create_with_capacity(size_t capacity) {
    arena a = arena_create();
    arena_add_block_exact(&a, capacity);
    return a;
}

void *arena_alloc(arena *a, size_t size) {
    ARENA_FOREACH(*a, current) {
        size_t remaining = current->b->capacity - current->b->size;
        if (remaining >= size) {
            return block_alloc(current->b, size);
        }
    }

    return block_alloc(arena_add_block(a, size), size);
}

void *arena_realloc(arena *a, void *data_block, size_t current_size, size_t new_size) {
    linked_block *best_fit = NULL;
    printf("Current size: %d, New size: %d\n", current_size, new_size);
    ARENA_FOREACH(*a, current) {
        size_t remaining = current->b->capacity - current->b->size;
        if ((data_block >= current->b->data) && ((uint8_t *)data_block + current_size == current->b->data + current->b->size)) {
            if (new_size <= remaining) {
                int32_t expansion = new_size - current_size;
                current->b->size += expansion;
                return data_block;
            }
        } else if ((remaining >= new_size) && ((best_fit == NULL) || (remaining < best_fit->b->capacity - best_fit->b->size))) {
            best_fit = current;
        }
    }
    
    if (best_fit != NULL) {
        void *new_data_block = block_alloc(best_fit->b, new_size);
        memcpy(new_data_block, data_block, new_size < current_size ? new_size : current_size);
        return new_data_block;
    } else {
        void *new_data_block = block_alloc(arena_add_block(a, new_size), new_size);
        memcpy(new_data_block, data_block, new_size < current_size ? new_size : current_size);
        return new_data_block;
    }
}

void arena_free(arena *a) {
    linked_block *current = a->first_block;
    while (current != NULL) {
        linked_block *next = current->next;
        linked_block_free(current);
        free(current);
        current = next;
    }

    a->first_block = NULL;
    a->last_block = NULL;
}

#endif