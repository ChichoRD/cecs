#ifndef DISPLACED_SET_H
#define DISPLACED_SET_H

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "list.h"
#include "range.h"

typedef struct displaced_set {
    list elements;
    exclusive_range index_range;
} displaced_set;

displaced_set displaced_set_create() {
    return (displaced_set){ .elements = list_create(), .index_range = {0, 0} };
}

displaced_set displaced_set_create_with_capacity(arena *a, size_t capacity) {
    return (displaced_set){ .elements = list_create_with_capacity(a, capacity), .index_range = {0, 0} };
}

bool displaced_set_contains(displaced_set *s, size_t index) {
    return exclusive_range_contains(s->index_range, index);
}

bool displaced_set_is_empty(displaced_set *s) {
    return exclusive_range_is_empty(s->index_range);
}

inline size_t displaced_set_list_index(displaced_set *s, size_t index) {
    assert(displaced_set_contains(s, index) && "index out of bounds");
    return index - s->index_range.start;
}

void *displaced_set_expand(displaced_set *s, arena *a, size_t index, size_t size) {
    if (displaced_set_is_empty(s)) {
        s->index_range = exclusive_range_singleton(index);
        return list_append_empty(&s->elements, a, 1, size);
    } else {
        exclusive_range expanded_range = exclusive_range_from(
            range_union(s->index_range.range, exclusive_range_singleton(index).range)
        );

        range_splitting expansion_ranges = exclusive_range_difference(
            expanded_range,
            s->index_range
        );

        exclusive_range range0 = exclusive_range_from(expansion_ranges.ranges[0]);
        exclusive_range range1 = exclusive_range_from(expansion_ranges.ranges[1]);
        s->index_range = expanded_range;
        if (!exclusive_range_is_empty(range0)) {
            size_t missing_count = exclusive_range_length(range0);
            return list_prepend_empty(&s->elements, a, missing_count, size);
        } else if (!exclusive_range_is_empty(range1)) {
            size_t missing_count = exclusive_range_length(range1);
            return list_append_empty(&s->elements, a, missing_count, size);
        } else {
            assert(false && "unreachable: should not have called expand with an index within bounds");
            return list_get(&s->elements, displaced_set_list_index(s, index), size);
        }
    }
}

void *displaced_set_set(displaced_set *s, arena *a, size_t index, void *element, size_t size) {
    if (!displaced_set_contains(s, index)) {
        return memcpy(displaced_set_expand(s, a, index, size), element, size);
    } else {
        return list_set(&s->elements, displaced_set_list_index(s, index), element, size);
    }
}
#define DISPLACED_SET_SET(type, set_ref, arena_ref, index, element_ref) \
    ((type *)displaced_set_set(set_ref, arena_ref, index, element_ref, sizeof(type)))

void *displaced_set_get(displaced_set *s, size_t index, size_t size) {
    if (!displaced_set_contains(s, index)) {
        assert(false && "index out of bounds");
        return NULL;
    } else {
        return list_get(&s->elements, displaced_set_list_index(s, index), size);
    }
}
#define DISPLACED_SET_GET(type, set_ref, index) \
    ((type *)displaced_set_get(set_ref, index, sizeof(type)))

bool displaced_set_remove(displaced_set *s, size_t index, void *out_removed_element, size_t size) {
    bool removed = displaced_set_contains(s, index);
    if (out_removed_element == NULL) {
        return removed;
    } else if (removed) {
        memcpy(out_removed_element, displaced_set_get(s, index, size), size);
    } else {
        memset(out_removed_element, 0, size);
    }

    return removed;
}

void displaced_set_clear(displaced_set *s) {
    list_clear(&s->elements);
    s->index_range = (exclusive_range){0, 0};
}

#endif