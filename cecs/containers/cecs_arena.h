#ifndef CECS_ARENA_H
#define CECS_ARENA_H

#include <stdint.h>
#include <stdbool.h>

#define CECS_DEFAULT_BLOCK_CAPACITY (8 * 1024)

typedef struct cecs_block {
    size_t size;
    size_t capacity;
    uint8_t *data;
    bool owns_data;
} cecs_block;

cecs_block cecs_block_create(size_t capacity);

cecs_block cecs_block_create_from_existing(size_t capacity, size_t size, uint8_t *data);

size_t cecs_block_alignment_padding_from_size(uint8_t *ptr, size_t structure_size);

bool cecs_block_can_alloc(const cecs_block *b, size_t structure_size);

void *cecs_block_alloc(cecs_block *b, size_t size);

void cecs_block_free(cecs_block *b);


typedef struct cecs_linked_block {
    cecs_block b;
    struct cecs_linked_block *next;
} cecs_linked_block;

cecs_linked_block cecs_linked_block_create(cecs_block b, cecs_linked_block *next);

void cecs_linked_block_free(cecs_linked_block *lb);


typedef struct cecs_arena {
    cecs_linked_block *first_block;
    cecs_linked_block *last_block;
} cecs_arena;

cecs_arena cecs_arena_create(void);

static cecs_linked_block *cecs_arena_prepend(cecs_arena *a, cecs_block b);

static cecs_block *cecs_arena_add_block_exact(cecs_arena *a, size_t capacity);

static cecs_block *cecs_arena_add_block(cecs_arena *a, size_t size);

cecs_arena cecs_arena_create_with_capacity(size_t capacity);

void *cecs_arena_alloc(cecs_arena *a, size_t size);

typedef enum cecs_arena_reallocation_strategy {
    arena_reallocate_none,
    arena_reallocate_in_place,
    arena_reallocate_split,
    arena_reallocate_fit,
    arena_reallocate_new
} cecs_arena_reallocation_strategy;

static cecs_arena_reallocation_strategy arena_realloc_find_fit(
    cecs_arena *a,
    void *data_block,
    size_t current_size,
    size_t new_size,
    cecs_linked_block **out_old_data_block,
    cecs_linked_block **out_fit
);

static bool cecs_arena_split_block_at(void *split, cecs_linked_block *block, cecs_linked_block **out_split_block);

void *cecs_arena_realloc(cecs_arena *a, void *data_block, size_t current_size, size_t new_size);

void cecs_arena_free(cecs_arena *a);

// TODO: pool allocator

typedef struct cecs_arena_dbg_info {
    size_t block_count;
    size_t owned_block_count;

    size_t total_capacity;
    size_t total_size;

    size_t largest_block_capacity;
    size_t largest_block_size;

    size_t smallest_block_capacity;
    size_t smallest_block_size;

    size_t largest_remaining_capacity;
} cecs_arena_dbg_info;

cecs_arena_dbg_info cecs_arena_get_dbg_info_compare_capacity(const cecs_arena *a);

cecs_arena_dbg_info cecs_arena_get_dbg_info_compare_size(const cecs_arena *a);

cecs_arena_dbg_info cecs_arena_get_dbg_info_pick_smallest(const cecs_arena *a);

#endif