#ifndef CECS_FLATMAP_H
#define CECS_FLATMAP_H

#include <stdint.h>
#include <assert.h>

#include "cecs_arena.h"
#include "cecs_dynamic_array.h"

typedef uint64_t cecs_flatmap_hash;
typedef uint8_t cecs_flatmap_low_hash;
typedef uint64_t cecs_flatmap_high_hash;

typedef struct cecs_flatmap_ctrl_occupied {
    bool occupied : 1;
    cecs_flatmap_low_hash low_hash : 7;
} cecs_flatmap_ctrl_occupied;
typedef struct cecs_flatmap_ctrl_non_occupied {
    bool occupied : 1;
    bool deleted : 1;
    uint8_t last_non_occupied : 6;
} cecs_flatmap_ctrl_non_occupied;
typedef struct cecs_flatmap_ctrl_any {
    bool occupied : 1;
    uint8_t uninitialised : 7;
} cecs_flatmap_ctrl_any;

typedef union cecs_flatmap_ctrl {
    cecs_flatmap_ctrl_occupied occupied;
    cecs_flatmap_ctrl_non_occupied non_occupied;
    cecs_flatmap_ctrl_any any;
} cecs_flatmap_ctrl;
static_assert(
    sizeof(cecs_flatmap_ctrl_occupied) == sizeof(cecs_flatmap_ctrl_occupied)
    && sizeof(cecs_flatmap_ctrl_occupied) == sizeof(cecs_flatmap_ctrl_any),
    "static error: flatmap ctrl size mismatch"
);
static_assert(
    sizeof(cecs_flatmap_ctrl) == sizeof(uint8_t),
    "static error: flatmap ctrl size must be 1 byte"
);

#define CECS_FLATMAP_LOW_HASH_BITS 7
#define CECS_FLATMPAP_LOW_HASH_MASK ((1 << CECS_FLATMAP_LOW_HASH_BITS) - 1)
extern const cecs_flatmap_low_hash cecs_flatmap_low_hash_mask;

#define CECS_FLATMAP_CTRL_NON_OCCUPIED_LAST_BITS 6
#define CECS_FLATMAP_CTRL_NON_OCCUPIED_LAST_MAX ((1 << CECS_FLATMAP_CTRL_NON_OCCUPIED_LAST_BITS) - 1)

extern const uint8_t cecs_flatmap_ctrl_non_occupied_last_max;

typedef union cecs_flatmap_hash_header {
    //cecs_flatmap_high_hash high_hash;
    cecs_flatmap_hash hash;
} cecs_flatmap_hash_header;

typedef struct cecs_flatmap {
    cecs_flatmap_ctrl *ctrl_and_hash_values;
    size_t count;
    size_t occupied;
} cecs_flatmap;

cecs_flatmap cecs_flatmap_create(void);
// cecs_flatmap cecs_flatmap_create_with_size(cecs_arena *a, size_t capacity, size_t value_size);

bool cecs_flatmap_get(
    const cecs_flatmap *m,
    const cecs_flatmap_hash hash,
    void **out_value,
    const size_t value_size
);

bool cecs_flatmap_add(
    cecs_flatmap *m,
    cecs_arena *a,
    const cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size,
    void **out_value
);

bool cecs_flatmap_remove(
    cecs_flatmap *m,
    cecs_arena *a,
    const cecs_flatmap_hash hash,
    void *out_removed_value,
    const size_t value_size
);

void *cecs_flatmap_get_or_add(
    cecs_flatmap *m,
    cecs_arena *a,
    const cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size
);

typedef struct cecs_flatmap_iterator {
    cecs_flatmap *map;
    size_t index;
} cecs_flatmap_iterator;

cecs_flatmap_iterator cecs_flatmap_iterator_create_at(cecs_flatmap *m, const size_t index);
bool cecs_flatmap_iterator_done(const cecs_flatmap_iterator *it);
bool cecs_flatmap_iterator_done_occupied(const cecs_flatmap_iterator *it, const size_t occupied_visited_count);
size_t cecs_flatmap_iterator_next(cecs_flatmap_iterator *it);
size_t cecs_flatmap_iterator_next_occupied(cecs_flatmap_iterator *it);

static inline size_t cecs_flatmap_iterator_current(const cecs_flatmap_iterator *it) {
    return it->index;
}
cecs_flatmap_hash_header *cecs_flatmap_iterator_current_hash(const cecs_flatmap_iterator *it, const size_t value_size);
void *cecs_flatmap_iterator_current_value(const cecs_flatmap_iterator *it, const size_t value_size);

#endif