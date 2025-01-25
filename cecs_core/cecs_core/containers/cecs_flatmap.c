#include <memory.h>
#include <stdlib.h>

#include "cecs_flatmap.h"

const uint8_t cecs_flatmap_ctrl_next_max = CECS_FLATMAP_CTRL_NEXT_MAX;
const cecs_flatmap_low_hash cecs_flatmap_low_hash_mask = CECS_FLATMPAP_LOW_HASH_MASK;

const cecs_flatmap_ctrl cecs_flatmap_ctrl_empty = { .next = 0, .low_hash = 0 };
const cecs_flatmap_ctrl cecs_flatmap_ctrl_deleted = { .next = CECS_FLATMAP_CTRL_NEXT_MAX, .low_hash = 0 };


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
    return (cecs_flatmap_hash_header *)(
        ((uint8_t *)cecs_flatmap_hash_values(m)) + index * cecs_flatmap_offset_of_next_value(value_size)
    );
}

static bool cecs_flatmap_find_or_next_empty(
    const cecs_flatmap *m,
    cecs_flatmap_hash hash,
    size_t *out_index,
    const size_t value_size,
    size_t *out_previous_index
) {
    if (m->count == 0) {
        *out_index = 0;
        return false;
    }

    const cecs_flatmap_low_hash low_hash = cecs_flatmap_hash_split(&hash);
    size_t index = hash % m->count;

    *out_previous_index = m->count;
    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    while (ctrl->next != cecs_flatmap_ctrl_empty.next) {
        *out_previous_index = index;
        if (ctrl->next == cecs_flatmap_ctrl_deleted.next) {
            ++index;
            ++ctrl;
            continue;
        }

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
    size_t *out_index,
    size_t *out_previous_index
) {
    if (m->count == 0) {
        *out_previous_index = 0;
        *out_index = 0;
        return false;
    }

    cecs_flatmap_hash_split(&hash);
    size_t index = hash % m->count;

    *out_previous_index = m->count;
    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    while (ctrl->next == 1) {
        *out_previous_index = index;
        ++index;
        ++ctrl;
    }

    if (ctrl->next == cecs_flatmap_ctrl_empty.next || ctrl->next == cecs_flatmap_ctrl_deleted.next) {
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
        .count = 0,
        .occupied = 0
    };
}

cecs_flatmap cecs_flatmpa_create_with_size(cecs_arena *a, size_t value_count, size_t value_size) {
    const size_t count_pow2m1 = cecs_next_pow2(value_count) - 1;
    return (cecs_flatmap){
        .ctrl_and_hash_values = cecs_arena_alloc(a, cecs_flatmap_offset_of_values_end(count_pow2m1, value_size)),
        .count = count_pow2m1,
        .occupied = 0
    };
}

bool cecs_flatmap_get(
    const cecs_flatmap *m,
    cecs_flatmap_hash hash,
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
    
    size_t i = 0;
    while (i < old_map.count) {
        cecs_flatmap_ctrl old_ctrl = *cecs_flatmap_ctrl_at(&old_map, i);
        if (old_ctrl.next != cecs_flatmap_ctrl_empty.next && old_ctrl.next != cecs_flatmap_ctrl_deleted.next) {
            cecs_flatmap_hash_header *old_hash_value = cecs_flatmap_hash_value_at(&old_map, i, value_size);
            cecs_flatmap_hash hash =
                (old_hash_value->high_hash << CECS_FLATMAP_LOW_HASH_BITS) | (cecs_flatmap_hash)old_ctrl.low_hash;

            void *out_value;
            bool added = cecs_flatmap_add(m, a, hash, old_hash_value + 1, value_size, &out_value);
            assert(added && "fatal error: flatmap rehash failed to add value");

            i += old_ctrl.next;
        } else {
            ++i;
        }
    }

    assert(m->occupied == old_map.occupied && "fatal error: flatmap rehash failed to maintain occupied count");
    cecs_arena_free(&arena);
    return new_count;
}

static inline size_t cecs_flatmap_rehash(cecs_flatmap *m, cecs_arena *a, const size_t value_size) {
    return cecs_flatmap_set_count_and_rehash(m, a, value_size, m->count);
}

bool cecs_flatmap_add(
    cecs_flatmap *m,
    cecs_arena *a,
    cecs_flatmap_hash hash,
    const void *value,
    const size_t value_size,
    void **out_value
) {
    size_t previous_index;
    size_t index;
    if (cecs_flatmap_find_or_next_empty(m, hash, &index, value_size, &previous_index)) {
        return false;
    }

    while (index + 1 >= m->count) {
        cecs_flatmap_set_count_and_rehash(m, a, value_size, ((m->count + 1) << 1) - 1);
        cecs_flatmap_find_next_empty(m, hash, &index, &previous_index);
    }
    assert(index + 1 < m->count && "error: flatmap insertion index must be below the last value index, last is reserved for empty");

    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(m, index);
    ctrl->low_hash = cecs_flatmap_hash_split(&hash);
    if (previous_index < m->count) {
        cecs_flatmap_ctrl *prev_ctrl = cecs_flatmap_ctrl_at(m, previous_index);
        if (prev_ctrl->next > 1) {
            ctrl->next = prev_ctrl->next - 1;
            prev_ctrl->next = 1;
        } else {
            ctrl->next = 1;
        }
    } else {
        ctrl->next = 1;
    }

    cecs_flatmap_hash_header *hash_value = cecs_flatmap_hash_value_at(m, index, value_size);
    hash_value->high_hash = hash;
    *out_value = memcpy(hash_value + 1, value, value_size);

    ++m->occupied;
    assert(m->occupied < m->count && "fatal error: flatmap occupied count must be below count");
    return true;
}

bool cecs_flatmap_remove(
    cecs_flatmap *m,
    cecs_arena *a,
    cecs_flatmap_hash hash,
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
    if (previous_index < m->count) {
        cecs_flatmap_ctrl *prev_ctrl = cecs_flatmap_ctrl_at(m, previous_index);
        if (prev_ctrl->next != cecs_flatmap_ctrl_empty.next && prev_ctrl->next != cecs_flatmap_ctrl_deleted.next) {
            uint_fast8_t new_previous_next = prev_ctrl->next + ctrl->next;
            assert(new_previous_next >= 2 && "fatal error: flatmap previous next value must be at least 2");
            
            if (new_previous_next < cecs_flatmap_ctrl_next_max) {
                prev_ctrl->next = new_previous_next;
            } else {
                bool has_previous = true;
                do {
                    cecs_flatmap_rehash(m, a, value_size);
                    bool found = cecs_flatmap_find_or_next_empty(m, hash, &index, value_size, &previous_index);
                    assert(found && "fatal error: flatmap rehash failed to find value after rehash");

                    ctrl = cecs_flatmap_ctrl_at(m, index);
                    new_previous_next = ctrl->next;

                    has_previous = previous_index < m->count;
                    if (has_previous) {
                        prev_ctrl = cecs_flatmap_ctrl_at(m, previous_index);
                        new_previous_next += prev_ctrl->next;
                    }
                    assert(
                        (has_previous || new_previous_next < cecs_flatmap_ctrl_next_max)
                        && "fatal error: in order to exceed next max, there must be a previous value to achieve this with a sum,"
                        "else ctrl has overflowed"
                    );
                } while (new_previous_next >= cecs_flatmap_ctrl_next_max);

                if (has_previous) {
                    prev_ctrl->next = new_previous_next;
                }
            }
            assert(new_previous_next < cecs_flatmap_ctrl_next_max && "fatal error: flatmap next value overflow");
        }
    }
    ctrl->next = cecs_flatmap_ctrl_deleted.next;

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

cecs_flatmap_iterator cecs_flatmap_iterator_create_at(cecs_flatmap *m, size_t index) {
    return (cecs_flatmap_iterator){
        .m = m,
        .index = index
    };
}

bool cecs_flatmap_iterator_done(const cecs_flatmap_iterator *it) {
    return it->index >= it->m->count;
}

bool cecs_flatmap_iterator_done_occupied(const cecs_flatmap_iterator *it, size_t occupied_visited) {
    return occupied_visited >= it->m->occupied || cecs_flatmap_iterator_done(it);
}

size_t cecs_flatmap_iterator_next(cecs_flatmap_iterator *it){
    return ++it->index;
}

size_t cecs_flatmap_iterator_next_occupied(cecs_flatmap_iterator *it) {
    cecs_flatmap_ctrl *ctrl = cecs_flatmap_ctrl_at(it->m, it->index); 
    bool deleted = false;
    do {
        deleted = ctrl->next == cecs_flatmap_ctrl_deleted.next;
        if (deleted) {
            ++it->index;
            ++ctrl;
        } else {
            uint_fast8_t next = max(1, ctrl->next);
            it->index += next;
            ctrl += next;
        }
    } while (
        !cecs_flatmap_iterator_done(it)
        && (deleted || ctrl->next == cecs_flatmap_ctrl_empty.next)
    );
    return it->index;
}

cecs_flatmap_hash_header *cecs_flatmap_iterator_current_hash(const cecs_flatmap_iterator *it, const size_t value_size) {
    return cecs_flatmap_hash_value_at(it->m, it->index, value_size);
}

void *cecs_flatmap_iterator_current_value(const cecs_flatmap_iterator *it, const size_t value_size) {
    return cecs_flatmap_iterator_current_hash(it, value_size) + 1;
}
