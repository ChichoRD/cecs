#include <memory.h>
#include <stdlib.h>

#include "cecs_flatmap.h"

const cecs_flatmap_low_hash cecs_flatmap_low_hash_mask = CECS_FLATMPAP_LOW_HASH_MASK;
const uint8_t cecs_flatmap_ctrl_non_occupied_last_max = CECS_FLATMAP_CTRL_NON_OCCUPIED_LAST_MAX;

static inline cecs_flatmap_low_hash cecs_flatmap_hash_low(const cecs_flatmap_hash hash) {
    return hash & cecs_flatmap_low_hash_mask;
}
static inline cecs_flatmap_hash cecs_flatmap_hash_high(const cecs_flatmap_hash hash) {
    return hash >> CECS_FLATMAP_LOW_HASH_BITS;
}
static inline cecs_flatmap_low_hash cecs_flatmap_hash_split(const cecs_flatmap_hash hash, cecs_flatmap_hash *out_high_hash) {
    *out_high_hash = cecs_flatmap_hash_high(hash);
    return cecs_flatmap_hash_low(hash);
}
[[maybe_unused]]
static inline cecs_flatmap_low_hash cecs_flatmap_hash_split_mut(cecs_flatmap_hash *in_out_hash) {
    cecs_flatmap_low_hash low_hash = cecs_flatmap_hash_low(*in_out_hash);
    *in_out_hash >>= CECS_FLATMAP_LOW_HASH_BITS;
    return low_hash;
}

static inline bool cecs_flatmap_count_is_pow2m1(size_t count) {
    return ((count + 1) & (count)) == 0;
}

static inline size_t cecs_flatmap_offset_of_ctrl_padding(size_t count) {
    return count * sizeof(cecs_flatmap_ctrl);
}

// TODO: add offsetof calculation to math and utility
static inline size_t cecs_flatmap_offset_of_values(size_t count) {
    static const size_t alignment_mask = 0x0F;
    const size_t padding_offset = cecs_flatmap_offset_of_ctrl_padding(count);
    return padding_offset + (((alignment_mask + 1) - (padding_offset & alignment_mask)) & alignment_mask);
}
static inline size_t cecs_flatmap_offset_of_next_value(size_t value_size) {
    static const size_t alignment_mask = 0x07;
    const size_t padding_offset = sizeof(cecs_flatmap_hash_header) + value_size;
    return padding_offset + (((alignment_mask + 1) - (padding_offset & alignment_mask)) & alignment_mask);
}
static inline size_t cecs_flatmap_offset_of_values_end(size_t count, size_t value_size) {
    return cecs_flatmap_offset_of_values(count) + count * cecs_flatmap_offset_of_next_value(value_size);
}

static inline cecs_flatmap_ctrl *cecs_flatmap_ctrl_at(const cecs_flatmap *m, size_t index) {
    assert(index < m->count && "error: flatmap ctrl index out of bounds");
    return m->ctrl_and_hash_values + index;
}
static inline cecs_flatmap_hash_header *cecs_flatmap_hash_values(const cecs_flatmap *m) {
    return (cecs_flatmap_hash_header *)(((uint8_t *)m->ctrl_and_hash_values) + cecs_flatmap_offset_of_values(m->count));
}
static inline cecs_flatmap_hash_header *cecs_flatmap_hash_value_at(const cecs_flatmap *m, size_t index, size_t value_size) {
    assert(index < m->count && "error: flatmap hash-value index out of bounds");
    return (cecs_flatmap_hash_header *)(
        ((uint8_t *)cecs_flatmap_hash_values(m)) + index * cecs_flatmap_offset_of_next_value(value_size)
    );
}

static bool cecs_flatmap_find_or_next_empty(
    const cecs_flatmap *m,
    const cecs_flatmap_hash hash,
    size_t *out_index,
    const size_t value_size,
    size_t *out_previous_index
) {
    *out_previous_index = m->count;
    if (m->count == 0) {
        *out_index = 0;
        return false;
    }

    cecs_flatmap_high_hash high_hash;
    const cecs_flatmap_low_hash low_hash = cecs_flatmap_hash_split(hash, &high_hash);
    size_t index = high_hash % m->count;

    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    while (ctrl->any.occupied || ctrl->non_occupied.deleted) {
        uint_fast8_t increment;
        if (ctrl->any.occupied) {
            increment = 1;
            if (
                ctrl->occupied.low_hash == low_hash
                && cecs_flatmap_hash_value_at(m, index, value_size)->hash == hash
            ){
                *out_index = index;
                return true;
            }
        } else {
            assert(ctrl->non_occupied.deleted && "fatal error: flatmap non-occupied ctrl must be deleted");
            increment = ctrl->non_occupied.last_non_occupied + 1;
        }
        
        *out_previous_index = index;
        index += increment;
        ctrl += increment;
    }

    *out_index = index;
    return false;
}

static bool cecs_flatmap_find_next_empty(
    const cecs_flatmap *m,
    const cecs_flatmap_hash hash,
    size_t *out_index,
    size_t *out_previous_index
) {
    *out_previous_index = m->count;
    if (m->count == 0) {
        *out_index = 0;
        return false;
    }

    size_t index = cecs_flatmap_hash_high(hash) % m->count;
    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    while (ctrl->any.occupied) {
        *out_previous_index = index;
        ++index;
        ++ctrl;
    }

    *out_index = index;
    return true;
}

cecs_flatmap cecs_flatmap_create(void) {
    return (cecs_flatmap){
        .ctrl_and_hash_values = NULL,
        .count = 0,
        .occupied = 0
    };
}

// cecs_flatmap cecs_flatmap_create_with_size(cecs_arena *a, size_t value_count, size_t value_size) {
//     const size_t count_pow2m1 = cecs_next_pow2(value_count) - 1;
//     return (cecs_flatmap){
//         .ctrl_and_hash_values = cecs_arena_alloc(a, cecs_flatmap_offset_of_values_end(count_pow2m1, value_size)),
//         .count = count_pow2m1,
//         .occupied = 0
//     };
// }

bool cecs_flatmap_get(
    const cecs_flatmap *m,
    const cecs_flatmap_hash hash,
    void **out_value,
    const size_t value_size
) {
    size_t previous_index;
    size_t index;
    if (cecs_flatmap_find_or_next_empty(m, hash, &index, value_size, &previous_index)) {
        *out_value = cecs_flatmap_hash_value_at(m, index, value_size) + 1;
        return true;
    } else {
        *out_value = NULL;
        return false;
    }
}

static size_t cecs_flatmap_set_count_and_rehash(cecs_flatmap *m, cecs_arena *a, const size_t value_size, const size_t new_count) {
    assert(cecs_flatmap_count_is_pow2m1(new_count) && "fatal error: flatmap new count is not power of 2 minus 1");
    
    size_t old_map_size = 0;
    cecs_arena arena = cecs_arena_create();
    cecs_flatmap old_map = (cecs_flatmap){
        .ctrl_and_hash_values = NULL,
        .count = m->count,
        .occupied = m->occupied
    };
    if (old_map.count > 0) {
        old_map_size = cecs_flatmap_offset_of_values_end(old_map.count, value_size);
        arena = cecs_arena_create_with_capacity(old_map_size);
        old_map.ctrl_and_hash_values = cecs_arena_alloc(&arena, old_map_size);
        memcpy(old_map.ctrl_and_hash_values, m->ctrl_and_hash_values, old_map_size);
    }

    const size_t new_map_size = cecs_flatmap_offset_of_values_end(new_count, value_size);
    m->ctrl_and_hash_values = cecs_arena_realloc(a, m->ctrl_and_hash_values, old_map_size, new_map_size);
    m->count = new_count;
    m->occupied = 0;
    memset(m->ctrl_and_hash_values, 0, new_map_size);
    
    cecs_flatmap_iterator it = cecs_flatmap_iterator_create_at(&old_map, 0);
    if (old_map.count > 0 && !old_map.ctrl_and_hash_values[0].any.occupied) {
        cecs_flatmap_iterator_next_occupied(&it);
    }

    size_t occupied_count = 0;
    while (!cecs_flatmap_iterator_done_occupied(&it, occupied_count)) {
        const cecs_flatmap_hash_header *old_hash_value = cecs_flatmap_hash_value_at(&old_map, it.index, value_size);

        void *out_value;
        bool added = cecs_flatmap_add(m, a, old_hash_value->hash, old_hash_value + 1, value_size, &out_value);
        assert(added && "fatal error: flatmap rehash failed to add value");
        ++occupied_count;

        cecs_flatmap_iterator_next_occupied(&it);
    }

    assert(m->occupied == old_map.occupied && "fatal error: flatmap rehash failed to maintain occupied count");
    cecs_arena_free(&arena);
    return new_count;
}

[[maybe_unused]]
static inline size_t cecs_flatmap_rehash(cecs_flatmap *m, cecs_arena *a, const size_t value_size) {
    return cecs_flatmap_set_count_and_rehash(m, a, value_size, m->count);
}

bool cecs_flatmap_add(
    cecs_flatmap *m,
    cecs_arena *a,
    const cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size,
    void **out_value
) {
    size_t previous_index;
    size_t index;
    if (cecs_flatmap_find_or_next_empty(m, hash, &index, value_size, &previous_index)) {
        *out_value = NULL;
        return false;
    }

    while (index + 1 >= m->count) {
        cecs_flatmap_set_count_and_rehash(m, a, value_size, ((m->count + 1) << 1) - 1);
        cecs_flatmap_find_next_empty(m, hash, &index, &previous_index);
    }
    assert(index + 1 < m->count && "error: flatmap insertion index must be below the last value index, last is reserved for empty");

    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    ctrl->any.occupied = true;
    ctrl->occupied.low_hash = cecs_flatmap_hash_low(hash);

    if (previous_index < index) {
        cecs_flatmap_ctrl *prev_ctrl = cecs_flatmap_ctrl_at(m, previous_index);
        if (!prev_ctrl->any.occupied) {
            prev_ctrl->non_occupied.last_non_occupied = index - previous_index - 1;
        }
    }

    cecs_flatmap_hash_header *hash_value = cecs_flatmap_hash_value_at(m, index, value_size);
    hash_value->hash = hash;
    *out_value = memcpy(hash_value + 1, value, value_size);

    ++m->occupied;
    assert(m->occupied < m->count && "fatal error: flatmap occupied count must be below count");
    return true;
}

bool cecs_flatmap_remove(
    cecs_flatmap *m,
    cecs_arena *a,
    const cecs_flatmap_hash hash,
    void *out_removed_value,
    const size_t value_size
) {
    size_t previous_index;
    size_t index;
    if (!cecs_flatmap_find_or_next_empty(m, hash, &index, value_size, &previous_index)) {
        return false;
    }
    assert(index + 1 < m->count && "error: flatmap removal index must be below the last value index, last is reserved for empty");
    
    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    ctrl->any.occupied = false;
    ctrl->non_occupied.deleted = true;
    if (ctrl[1].any.occupied) {
        ctrl->non_occupied.last_non_occupied = 0;
    } else {
        ctrl->non_occupied.last_non_occupied =
            min(cecs_flatmap_ctrl_non_occupied_last_max, ctrl[1].non_occupied.last_non_occupied + 1);
    }

    if (previous_index < index) {
        cecs_flatmap_ctrl *prev_ctrl = cecs_flatmap_ctrl_at(m, previous_index);
        if (!prev_ctrl->any.occupied) {
            prev_ctrl->non_occupied.last_non_occupied =
                min(cecs_flatmap_ctrl_non_occupied_last_max, ctrl->non_occupied.last_non_occupied + index - previous_index);
        }
    }

    cecs_flatmap_hash_header *hash_value = cecs_flatmap_hash_value_at(m, index, value_size);
    memcpy(out_removed_value, hash_value + 1, value_size);

    --m->occupied;
    if (m->occupied < m->count >> 3) {
        cecs_flatmap_set_count_and_rehash(m, a, value_size, ((m->count + 1) >> 1) - 1);
    }
    return true;
}

void *cecs_flatmap_get_or_add(
    cecs_flatmap *m,
    cecs_arena *a,
    const cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size
) {
    void *out_value;
    if (!cecs_flatmap_get(m, hash, &out_value, value_size)) {
        cecs_flatmap_add(m, a, hash, value, value_size, &out_value);
    }
    return out_value;
}

cecs_flatmap_iterator cecs_flatmap_iterator_create_at(cecs_flatmap *m, const size_t index) {
    return (cecs_flatmap_iterator){
        .map = m,
        .index = index
    };
}

bool cecs_flatmap_iterator_done(const cecs_flatmap_iterator *it) {
    return it->index >= it->map->count;
}

bool cecs_flatmap_iterator_done_occupied(const cecs_flatmap_iterator *it, const size_t occupied_visited) {
    return occupied_visited >= it->map->occupied || cecs_flatmap_iterator_done(it);
}

size_t cecs_flatmap_iterator_next(cecs_flatmap_iterator *it){
    return ++it->index;
}

size_t cecs_flatmap_iterator_next_occupied(cecs_flatmap_iterator *it) {
    ++it->index;
    cecs_flatmap_ctrl *ctrl = it->map->ctrl_and_hash_values + it->index;

    while (
        !cecs_flatmap_iterator_done(it)
        && !ctrl->any.occupied
    ) {
        it->index += ctrl->non_occupied.last_non_occupied + 1;
        ctrl += ctrl->non_occupied.last_non_occupied + 1;
    }
    return it->index;
}

cecs_flatmap_hash_header *cecs_flatmap_iterator_current_hash(const cecs_flatmap_iterator *it, const size_t value_size) {
    return cecs_flatmap_hash_value_at(it->map, it->index, value_size);
}

void *cecs_flatmap_iterator_current_value(const cecs_flatmap_iterator *it, const size_t value_size) {
    return cecs_flatmap_iterator_current_hash(it, value_size) + 1;
}
