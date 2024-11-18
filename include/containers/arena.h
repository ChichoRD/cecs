#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#define DEFAULT_BLOCK_CAPACITY (8 * 1024)

typedef struct block {
    size_t size;
    size_t capacity;
    uint8_t *data;
    bool owns_data;
} block;

block block_create(size_t capacity);

block block_create_from_existing(size_t capacity, size_t size, uint8_t *data);

size_t block_alignment_padding_from_size(uint8_t *ptr, size_t structure_size);

bool block_can_alloc(const block *b, size_t structure_size);

void *block_alloc(block *b, size_t size);

void block_free(block *b);


typedef struct linked_block {
    block b;
    struct linked_block *next;
} linked_block;

linked_block linked_block_create(block b, linked_block *next);

void linked_block_free(linked_block *lb);


typedef struct arena {
    linked_block *first_block;
    linked_block *last_block;
} arena;

arena arena_create(void);

static linked_block *arena_prepend(arena *a, block b);

static block *arena_add_block_exact(arena *a, size_t capacity);

static block *arena_add_block(arena *a, size_t size);

arena arena_create_with_capacity(size_t capacity);

void *arena_alloc(arena *a, size_t size);

typedef enum arena_reallocation_strategy {
    arena_reallocate_none,
    arena_reallocate_in_place,
    arena_reallocate_split,
    arena_reallocate_fit,
    arena_reallocate_new
} arena_reallocation_strategy;

static arena_reallocation_strategy arena_realloc_find_fit(
    arena *a,
    void *data_block,
    size_t current_size,
    size_t new_size,
    linked_block **out_old_data_block,
    linked_block **out_fit
);

static bool arena_split_block_at(void *split, linked_block *block, linked_block **out_split_block);

void *arena_realloc(arena *a, void *data_block, size_t current_size, size_t new_size);

void arena_free(arena *a);

// TODO: pool allocator

typedef struct arena_dbg_info {
    size_t block_count;
    size_t owned_block_count;

    size_t total_capacity;
    size_t total_size;

    size_t largest_block_capacity;
    size_t largest_block_size;

    size_t smallest_block_capacity;
    size_t smallest_block_size;

    size_t largest_remaining_capacity;
} arena_dbg_info;

arena_dbg_info arena_get_dbg_info_compare_capacity(const arena *a);

arena_dbg_info arena_get_dbg_info_compare_size(const arena *a);

arena_dbg_info arena_get_dbg_info_pick_smallest(const arena *a);

#endif