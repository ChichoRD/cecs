#include <memory.h>

#include "cecs_flatmap.h"

const uint8_t cecs_flatmap_ctrl_next_max = CECS_FLATMAP_CTRL_NEXT_MAX;
const cecs_flatmap_low_hash cecs_flatmap_low_hash_mask = CECS_FLATMPAP_LOW_HASH_MASK;

const cecs_flatmap_ctrl cecs_flatmap_ctrl_empty = { .next = 0, .low_hash = 0 };


static inline cecs_flatmap_low_hash cecs_flatmap_hash_split(cecs_flatmap_hash *in_out_hash) {
    cecs_flatmap_low_hash low_hash = *in_out_hash & cecs_flatmap_low_hash_mask;
    *in_out_hash >>= CECS_FLATMAP_LOW_HASH_BITS;
    return low_hash;
}

static inline bool cecs_flatmap_count_is_pow2m1(size_t count) {
    return ((count + 1) & (count)) == 0;
}

static inline bool cecs_next_pow2(size_t n) {
    return n && !(n & (n - 1));
}

static inline size_t cecs_flatmap_offset_of_ctrl_padding(size_t count) {
    return count * sizeof(cecs_flatmap_ctrl);
}

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
    return cecs_flatmap_hash_values(m) + index * cecs_flatmap_offset_of_next_value(value_size);
}

static bool cecs_flatmap_find_or_next_empty(
    const cecs_flatmap *m,
    cecs_flatmap_hash hash,
    size_t *out_index,
    const size_t value_size
) {
    if (m->count == 0) {
        *out_index = 0;
        return false;
    }

    const cecs_flatmap_low_hash low_hash = cecs_flatmap_hash_split(&hash);
    size_t index = hash % m->count;

    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    while (ctrl->next != cecs_flatmap_ctrl_empty.next) {
        if (ctrl->low_hash == low_hash) {
            const cecs_flatmap_hash_header *hash_value = cecs_flatmap_hash_value_at(m, index, value_size);
            if (hash_value->high_hash == hash) {
                *out_index = index;
                return true;
            }
        }

        index += ctrl->next;
        ctrl += ctrl->next;
    }

    *out_index = index;
    return false;
}

static bool cecs_flatmap_find_next_empty(
    const cecs_flatmap *m,
    cecs_flatmap_hash hash,
    size_t *out_index
) {
    if (m->count == 0) {
        *out_index = 0;
        return false;
    }

    const cecs_flatmap_low_hash low_hash = cecs_flatmap_hash_split(&hash);
    size_t index = hash % m->count;

    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    while (ctrl->next == 1) {
        ++index;
        ++ctrl;
    }

    if (ctrl->next == cecs_flatmap_ctrl_empty.next) {
        *out_index = index;
        return true;
    } else {
        *out_index = index + 1;
        return true;
    }
}

cecs_flatmap cecs_flatmap_create(void) {
    return (cecs_flatmap){
        .ctrl_and_hash_values = NULL,
        .count = 0
    };
}

cecs_flatmap cecs_flatmpa_create_with_size(cecs_arena *a, size_t value_count, size_t value_size) {
    const size_t count_pow2m1 = cecs_next_pow2(value_count) - 1;
    return (cecs_flatmap){
        .ctrl_and_hash_values = cecs_arena_alloc(a, cecs_flatmap_offset_of_values_end(count_pow2m1, value_size)),
        .count = count_pow2m1
    };
}

bool cecs_flatmap_get(
    const cecs_flatmap *m,
    cecs_flatmap_hash hash,
    void **out_value,
    const size_t value_size
) {
    size_t index;
    if (cecs_flatmap_find_or_next_empty(m, hash, &index, value_size)) {
        *out_value = cecs_flatmap_hash_value_at(m, index, value_size);
        return true;
    } else {
        return false;
    }
}

static size_t cecs_flatmap_grow_and_rehash(cecs_flatmap *m, cecs_arena *a, const size_t value_size) {
    assert(cecs_flatmap_count_is_pow2m1(m->count) && "fatal error: flatmap count is not power of 2 minus 1");
    const size_t new_count = ((m->count + 1) << 1) - 1;

    size_t old_map_size = 0;
    cecs_arena arena = cecs_arena_create();
    cecs_flatmap old_map = (cecs_flatmap){
        .ctrl_and_hash_values = NULL,
        .count = m->count
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
    memset(m->ctrl_and_hash_values, 0, new_map_size);
    
    for (size_t i = 0; i < old_map.count; i++) {
        cecs_flatmap_ctrl *old_ctrl = cecs_flatmap_ctrl_at(&old_map, i);
        if (old_ctrl->next != cecs_flatmap_ctrl_empty.next) {
            cecs_flatmap_hash_header *old_hash_value = cecs_flatmap_hash_value_at(&old_map, i, value_size);
            cecs_flatmap_hash hash = old_hash_value->high_hash << CECS_FLATMAP_LOW_HASH_BITS | old_ctrl->low_hash;

            void *out_value;
            cecs_flatmap_add(m, a, hash, old_hash_value + 1, value_size, &out_value);
        }
    }

    cecs_arena_free(&arena);
    return new_count;
}

bool cecs_flatmap_add(
    cecs_flatmap *m,
    cecs_arena *a,
    cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size,
    void **out_value
) {
    size_t index;
    if (cecs_flatmap_find_or_next_empty(m, hash, &index, value_size)) {
        return false;
    }

    while (index + 1 >= m->count) {
        cecs_flatmap_grow_and_rehash(m, a, value_size);
        cecs_flatmap_find_next_empty(m, hash, &index);
    }
    assert(index + 1 < m->count && "error: flatmap insertion index must be below the last value index, last is reserved for empty");

    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    cecs_flatmap_ctrl *prev_ctrl = ctrl - 1;

    ctrl->low_hash = cecs_flatmap_hash_split(&hash);
    if (index > 1 && prev_ctrl->next != cecs_flatmap_ctrl_empty.next) {
        if (prev_ctrl->next == 1) {
            ctrl->next = 1;
        } else {
            ctrl->next = prev_ctrl->next - 1;
            prev_ctrl->next = 1;
        }
    } else {
        ctrl->next = 1;
        prev_ctrl = NULL;
    }

    cecs_flatmap_hash_header *hash_value = cecs_flatmap_hash_value_at(m, index, value_size);
    hash_value->high_hash = hash;
    *out_value = memcpy(hash_value + 1, value, value_size);
    return true;
}

bool cecs_flatmap_remove(
    cecs_flatmap *m,
    cecs_arena *a,
    cecs_flatmap_hash hash,
    void *out_removed_value,
    const size_t value_size
) {
    size_t index;
    if (!cecs_flatmap_find_or_next_empty(m, hash, &index, value_size)) {
        return false;
    }
    assert(index + 1 < m->count && "error: flatmap removal index must be below the last value index, last is reserved for empty");
    
    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    cecs_flatmap_ctrl *prev_ctrl = ctrl - 1;

    if (index > 1 && prev_ctrl->next != cecs_flatmap_ctrl_empty.next) {
        prev_ctrl->next += ctrl->next;
        ctrl->next = cecs_flatmap_ctrl_empty.next;
    } else {
        prev_ctrl = NULL;
        ctrl->next = cecs_flatmap_ctrl_empty.next;
    }

    cecs_flatmap_hash_header *hash_value = cecs_flatmap_hash_value_at(m, index, value_size);
    memcpy(out_removed_value, hash_value + 1, value_size);
    return true;
}

void *cecs_flatmap_get_or_add(
    cecs_flatmap *m,
    cecs_arena *a,
    cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size
) {
    void *out_value;
    if (!cecs_flatmap_get(m, hash, &out_value, value_size)) {
        cecs_flatmap_add(m, a, hash, value, value_size, &out_value);
    }
    return out_value;
}