#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
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
    assert(remaining >= size);

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

#define ARENA_FOREACH(a, lb) for (linked_block *(lb) = (a).first_block; (lb) != NULL; (lb) = lb->next)

arena arena_create(void) {
    arena a;
    a.first_block = NULL;
    a.last_block = NULL;
    return a;
}

static void arena_prepend(arena *a, block *b) {
    linked_block *new = (linked_block *)malloc(sizeof(linked_block));
    *new = linked_block_create(b, a->first_block);

    a->first_block = new;
    if (a->last_block == NULL) {
        a->last_block = new;
    }
}

void *arena_alloc(arena *a, size_t size) {
    ARENA_FOREACH(*a, current) {
        size_t remaining = current->b->capacity - current->b->size;
        if (remaining >= size) {
            return block_alloc(current->b, size);
        }
    }

    int capacity = size > DEFAULT_BLOCK_CAPACITY ? size : DEFAULT_BLOCK_CAPACITY;
    block *b = (block *)malloc(sizeof(block));
    *b = block_create(capacity);
    arena_prepend(a, b);

    return block_alloc(a->first_block->b, size);
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