#ifndef CECS_FLATMAP_H
#define CECS_FLATMAP_H

#include <stdint.h>
#include <assert.h>

#include "cecs_arena.h"
#include "cecs_dynamic_array.h"

typedef uint64_t cecs_flatmap_hash;
typedef uint8_t cecs_flatmap_low_hash;
typedef uint64_t cecs_flatmap_high_hash;

typedef struct cecs_flatmap_ctrl {
    uint8_t next : 4;
    cecs_flatmap_low_hash low_hash : 4;
} cecs_flatmap_ctrl;

#define CECS_FLATMAP_CTRL_NEXT_BITS_LOG2 2
#define CECS_FLATMAP_CTRL_NEXT_BITS (1 << CECS_FLATMAP_CTRL_NEXT_BITS_LOG2)
#define CECS_FLATMAP_CTRL_NEXT_BITS_VALUE 4
static_assert(
    CECS_FLATMAP_CTRL_NEXT_BITS == CECS_FLATMAP_CTRL_NEXT_BITS_VALUE,
    "static error: next empty bits value mismatch"
);
#define CECS_FLATMAP_CTRL_NEXT_MAX ((1 << CECS_FLATMAP_CTRL_NEXT_BITS) - 1)
extern const uint8_t cecs_flatmap_ctrl_next_max;
extern const cecs_flatmap_ctrl cecs_flatmap_ctrl_empty;

#define CECS_FLATMAP_LOW_HASH_BITS_LOG2 2
#define CECS_FLATMAP_LOW_HASH_BITS (1 << CECS_FLATMAP_LOW_HASH_BITS_LOG2)
#define CECS_FLATMAP_LOW_HASH_BITS_VALUE 4
static_assert(
    CECS_FLATMAP_LOW_HASH_BITS == CECS_FLATMAP_LOW_HASH_BITS_VALUE,
    "static error: low hash bits value mismatch"
);
#define CECS_FLATMPAP_LOW_HASH_MASK ((1 << CECS_FLATMAP_LOW_HASH_BITS) - 1)
extern const cecs_flatmap_low_hash cecs_flatmap_low_hash_mask;

typedef union cecs_flatmap_hash_header {
    cecs_flatmap_high_hash high_hash;
    cecs_flatmap_hash hash;
} cecs_flatmap_hash_header;

typedef struct cecs_flatmap {
    cecs_flatmap_ctrl *ctrl_and_hash_values;
    size_t count;
} cecs_flatmap;

cecs_flatmap cecs_flatmap_create(void);
cecs_flatmap cecs_flatmpa_create_with_size(cecs_arena *a, size_t capacity, size_t value_size);

bool cecs_flatmap_get(
    const cecs_flatmap *m,
    cecs_flatmap_hash hash,
    void **out_value,
    const size_t value_size
);

bool cecs_flatmap_add(
    cecs_flatmap *m,
    cecs_arena *a,
    cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size,
    void **out_value
);

bool cecs_flatmap_remove(
    cecs_flatmap *m,
    cecs_arena *a,
    cecs_flatmap_hash hash,
    void *out_removed_value,
    const size_t value_size
);

void *cecs_flatmap_get_or_add(
    cecs_flatmap *m,
    cecs_arena *a,
    cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size
);

#endif