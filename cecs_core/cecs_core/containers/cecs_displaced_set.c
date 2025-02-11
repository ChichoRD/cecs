#include <string.h>
#include <stdlib.h>
#include "cecs_displaced_set.h"

cecs_sentinel_set cecs_sentinel_set_create(void) {
    return (cecs_sentinel_set) {
        .values = cecs_dynamic_array_create(),
        .index_range = { 0, 0 },
        .first_last_set = { PTRDIFF_MAX, 0 }
    };
}
cecs_sentinel_set cecs_sentinel_set_create_with_capacity(cecs_arena* a, const size_t capacity) {
    return (cecs_sentinel_set) {
        .values = cecs_dynamic_array_create_with_capacity(a, capacity),
        .index_range = { 0, 0 },  
        .first_last_set = { PTRDIFF_MAX, 0 }
    };
}

extern inline bool cecs_sentinel_set_contains_index(const cecs_sentinel_set *s, const size_t index); 
static inline size_t cecs_sentinel_set_value_index(const cecs_sentinel_set *s, const size_t index) {
    assert(cecs_sentinel_set_contains_index(s, index) && "index out of bounds");
    return index - s->index_range.start;
}
bool cecs_sentinel_set_is_empty(const cecs_sentinel_set* s) {
    return cecs_exclusive_range_is_empty(s->index_range);
}

extern inline bool cecs_sentinel_set_contains_range(const cecs_sentinel_set *s, const cecs_inclusive_range range);
void *cecs_sentinel_set_expand_to_include(
    cecs_sentinel_set* s,
    cecs_arena* a,
    const cecs_inclusive_range range,
    const size_t size,
    const uint_fast8_t absent_pattern
) {
    if (cecs_sentinel_set_is_empty(s)) {
        s->index_range = cecs_exclusive_range_from_inclusive(range.range);
        return memset(
            cecs_dynamic_array_extend(&s->values, a, cecs_exclusive_range_length(s->index_range), size),
            (int)absent_pattern,
            size
        );
    } else if (!cecs_sentinel_set_contains_range(s, range)) {
        cecs_exclusive_range expanded_range = cecs_exclusive_range_from(
            cecs_range_union(s->index_range.range, cecs_exclusive_range_from_inclusive(range.range).range)
        );

        cecs_range_splitting expansion_ranges = cecs_exclusive_range_difference(
            expanded_range,
            s->index_range
        );

        cecs_exclusive_range range0 = cecs_exclusive_range_from(expansion_ranges.ranges[0]);
        cecs_exclusive_range range1 = cecs_exclusive_range_from(expansion_ranges.ranges[1]);
        s->index_range = expanded_range;

        void *first_expanded = NULL;
        if (!cecs_exclusive_range_is_empty(range0)) {
            size_t missing_count = cecs_exclusive_range_length(range0);
            first_expanded = 
                memset(cecs_dynamic_array_extend_within(&s->values, a, 0, missing_count, size), (int)absent_pattern, missing_count * size);
        }
        if (!cecs_exclusive_range_is_empty(range1)) {
            size_t missing_count = cecs_exclusive_range_length(range1);
            void *expanded =
                memset(cecs_dynamic_array_extend(&s->values, a, missing_count, size), (int)absent_pattern, missing_count * size);
            if (first_expanded == NULL) {
                first_expanded = expanded;
            }
        }
        
        if (first_expanded == NULL) {
            assert(false && "unreachable: range splitting error");
            exit(EXIT_FAILURE);
        }
        return first_expanded;
    } else {
        size_t array_index = cecs_sentinel_set_value_index(s, range.start);
        assert(
            array_index < cecs_dynamic_array_count_of_size(&s->values, size)
            && "error: index out of bounds"
        );
        return cecs_dynamic_array_get_range_mut(&s->values, array_index, cecs_inclusive_range_length(range), size);
    }
}

static inline cecs_inclusive_range cecs_sentinel_set_include_first_last_set(cecs_inclusive_range first_last_set, const cecs_inclusive_range include) {
    if (include.end > first_last_set.end) {
        first_last_set.end = include.end;
    }
    if (include.start < first_last_set.start) {
        first_last_set.start = include.start;
    }
    return first_last_set;
}

static inline cecs_inclusive_range cecs_sentinel_set_exclude_first_last_set(cecs_inclusive_range first_last_set, const cecs_inclusive_range exclude) {
    if (cecs_inclusive_range_contains(first_last_set, exclude.start)) {
        first_last_set.start = exclude.end + 1;
    }
    if (cecs_inclusive_range_contains(first_last_set, exclude.end)) {
        first_last_set.end = exclude.start - 1;
    }
    return first_last_set;
}

void *cecs_sentinel_set_set_inbounds(cecs_sentinel_set *s, const size_t index, const void *element, const size_t size) {
    s->first_last_set = cecs_sentinel_set_include_first_last_set(s->first_last_set, cecs_inclusive_range_singleton(index));
    return cecs_dynamic_array_set(&s->values, cecs_sentinel_set_value_index(s, index), element, size);
}
void *cecs_sentinel_set_set_range_inbounds(cecs_sentinel_set *s, const cecs_inclusive_range range, const void *elements, const size_t size) {
    assert(cecs_sentinel_set_contains_index(s, range.end) && "error: range end out of bounds");
    s->first_last_set = cecs_sentinel_set_include_first_last_set(s->first_last_set, range);
    return cecs_dynamic_array_set_range(
        &s->values,
        cecs_sentinel_set_value_index(s, range.start),
        elements,
        cecs_inclusive_range_length(range),
        size
    );
}
void *cecs_sentinel_set_set_copy_range_inbounds(cecs_sentinel_set *s, const cecs_inclusive_range range, const void *single_src, const size_t size) {
    assert(cecs_sentinel_set_contains_index(s, range.end) && "error: range end out of bounds");
    s->first_last_set = cecs_sentinel_set_include_first_last_set(s->first_last_set, range);
    return cecs_dynamic_array_set_copy_range(
        &s->values,
        cecs_sentinel_set_value_index(s, range.start),
        single_src,
        cecs_inclusive_range_length(range),
        size
    );
}

void* cecs_sentinel_set_get_inbounds_mut(cecs_sentinel_set* s, const size_t index, const size_t size) {
    return cecs_dynamic_array_get_mut(&s->values, cecs_sentinel_set_value_index(s, index), size);
}
const void *cecs_sentinel_set_get_inbounds(const cecs_sentinel_set *s, const size_t index, const size_t size) {
    return cecs_dynamic_array_get(&s->values, cecs_sentinel_set_value_index(s, index), size);
}
void *cecs_sentinel_set_get_range_inbounds_mut(cecs_sentinel_set *s, const cecs_inclusive_range range, const size_t size) {
    assert(cecs_sentinel_set_contains_index(s, range.end) && "error: range end out of bounds");
    return cecs_dynamic_array_get_range_mut(&s->values, cecs_sentinel_set_value_index(s, range.start), cecs_inclusive_range_length(range), size);
}
const void *cecs_sentinel_set_get_range_inbounds(const cecs_sentinel_set *s, const cecs_inclusive_range range, const size_t size) {
    assert(cecs_sentinel_set_contains_index(s, range.end) && "error: range end out of bounds");
    return cecs_dynamic_array_get_range(&s->values, cecs_sentinel_set_value_index(s, range.start), cecs_inclusive_range_length(range), size);
}


bool cecs_sentinel_set_remove(
    cecs_sentinel_set* s,
    cecs_arena *a,
    const size_t index,
    void* out_removed_element,
    const size_t size,
    const uint_fast8_t absent_pattern
) {
    assert(out_removed_element != NULL && "out_removed_element must not be NULL");

    if (!cecs_inclusive_range_contains(s->first_last_set, index)) {
        memset(out_removed_element, absent_pattern, size);
        return false;
    } else {
        const size_t array_index = cecs_sentinel_set_value_index(s, index);
        void *removed = cecs_dynamic_array_get_mut(&s->values, array_index, size);
        memcpy(out_removed_element, removed, size);
        memset(removed, absent_pattern, size);
        s->first_last_set = cecs_sentinel_set_exclude_first_last_set(s->first_last_set, cecs_inclusive_range_singleton(index));

        const size_t present_count = cecs_inclusive_range_length(s->first_last_set);
        if (present_count < (size_t)cecs_exclusive_range_length(s->index_range) / 2) {
            const size_t first_present_index = cecs_sentinel_set_value_index(s, s->first_last_set.start);
            memmove(
                s->values.values,
                s->values.values + first_present_index * size,
                present_count * size
            );
            cecs_dynamic_array_truncate(&s->values, a, present_count, size);
            s->index_range = cecs_exclusive_range_index_count(s->first_last_set.start, present_count);
        }
        return true;
    }
}
void cecs_sentinel_set_clear(cecs_sentinel_set* s) {
    cecs_dynamic_array_clear(&s->values);
    s->index_range = (cecs_exclusive_range){ 0, 0 };
    s->first_last_set = (cecs_inclusive_range){ PTRDIFF_MAX, 0 };
}
