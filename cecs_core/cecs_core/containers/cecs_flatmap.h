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
    bool occupied : 1;
    uint8_t next_different : 3;
    cecs_flatmap_low_hash low_hash : 4;
} cecs_flatmap_ctrl;

#define CECS_FLATMAP_CTRL_NEXT_BITS 3
#define CECS_FLATMAP_CTRL_NEXT_MAX ((1 << CECS_FLATMAP_CTRL_NEXT_BITS) - 1)

extern const uint8_t cecs_flatmap_ctrl_next_max;
extern const cecs_flatmap_ctrl cecs_flatmap_ctrl_empty;
extern const cecs_flatmap_ctrl cecs_flatmap_ctrl_deleted;

#define CECS_FLATMAP_LOW_HASH_BITS_LOG2 2
#define CECS_FLATMAP_LOW_HASH_BITS (1 << CECS_FLATMAP_LOW_HASH_BITS_LOG2)
#define CECS_FLATMAP_LOW_HASH_BITS_VALUE 4
static_assert(
    CECS_FLATMAP_LOW_HASH_BITS == CECS_FLATMAP_LOW_HASH_BITS_VALUE,
    "static error: low hash bits value mismatch"
);
#define CECS_FLATMPAP_LOW_HASH_MASK ((1 << CECS_FLATMAP_LOW_HASH_BITS) - 1)
extern const cecs_flatmap_low_hash cecs_flatmap_low_hash_mask;

typedef struct cecs_flatmap_ctrl_non_occupied {
    bool occupied : 1;
    uint8_t next_different : 7;
} cecs_flatmap_ctrl_non_occupied;
static_assert(
    sizeof(cecs_flatmap_ctrl) == sizeof(cecs_flatmap_ctrl_non_occupied),
    "static error: flatmap ctrl size mismatch"
);

#define CECS_FLATMAP_CTRL_NON_OCCUPIED_NEXT_BITS 7
#define CECS_FLATMAP_CTRL_NON_OCCUPIED_NEXT_MAX ((1 << CECS_FLATMAP_CTRL_NON_OCCUPIED_NEXT_BITS) - 1)
#define CECS_FLATMAP_CTRL_NON_OCCUPIED_NEXT_MAX_HALF (CECS_FLATMAP_CTRL_NON_OCCUPIED_NEXT_MAX >> 1)

extern const uint8_t cecs_flatmap_ctrl_non_occupied_next_max;
extern const cecs_flatmap_ctrl_non_occupied cecs_flatmap_ctrl_non_occupied_empty;
extern const cecs_flatmap_ctrl_non_occupied cecs_flatmap_ctrl_non_occupied_deleted;


typedef union cecs_flatmap_hash_header {
    cecs_flatmap_high_hash high_hash;
    cecs_flatmap_hash hash;
} cecs_flatmap_hash_header;

typedef struct cecs_flatmap {
    cecs_flatmap_ctrl *ctrl_and_hash_values;
    size_t count;
    size_t occupied;
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

typedef struct cecs_flatmap_iterator {
    cecs_flatmap *m;
    size_t index;
} cecs_flatmap_iterator;

cecs_flatmap_iterator cecs_flatmap_iterator_create_at(cecs_flatmap *m, size_t index);
bool cecs_flatmap_iterator_done(const cecs_flatmap_iterator *it);
bool cecs_flatmap_iterator_done_occupied(const cecs_flatmap_iterator *it, size_t occupied_visited);
size_t cecs_flatmap_iterator_next(cecs_flatmap_iterator *it);
size_t cecs_flatmap_iterator_next_occupied(cecs_flatmap_iterator *it);

static inline size_t cecs_flatmap_iterator_current(const cecs_flatmap_iterator *it) {
    return it->index;
}
cecs_flatmap_hash_header *cecs_flatmap_iterator_current_hash(const cecs_flatmap_iterator *it, const size_t value_size);
void *cecs_flatmap_iterator_current_value(const cecs_flatmap_iterator *it, const size_t value_size);

#endif