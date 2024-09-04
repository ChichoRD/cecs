#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "tagged_union.h"
#include "range.h"
#include "arena.h"
#include "list.h"

typedef OPTION_STRUCT(size_t, size_t) dense_index;
typedef OPTION_STRUCT(void *, optional_element) optional_element;
typedef struct sparse_set {
    list data;
    list indices;
    list keys;
    exclusive_range key_range;
} sparse_set;

sparse_set sparse_set_create(arena *a, size_t element_capacity, size_t element_size) {
    return (sparse_set) {
        .data = list_create(a, element_capacity * element_size),
        .indices = list_create(a, sizeof(dense_index) * element_capacity),
        .keys = list_create(a, sizeof(size_t) * element_capacity),
        .key_range = (exclusive_range) { 0, 0 }
    };
}

bool sparse_set_contains(const sparse_set *s, size_t key) {
    return exclusive_range_contains(s->key_range, key)
        && OPTION_IS_SOME(*LIST_GET(dense_index, &s->indices, key - s->key_range.start));
}

optional_element sparse_set_get(const sparse_set *s, size_t key, size_t element_size) {
    if (!exclusive_range_contains(s->key_range, key)) {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }

    size_t list_index = key - s->key_range.start;
    dense_index index = *LIST_GET(dense_index, &s->indices, list_index);
    if (OPTION_IS_SOME(index)) {
        return OPTION_CREATE_SOME_STRUCT(
            optional_element, 
            list_get(&s->data, OPTION_GET(size_t, index), element_size)
        );
    } else {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }
}

bool sparse_set_is_empty(const sparse_set *s) {
    return exclusive_range_is_empty(s->key_range);
}

dense_index *sparse_set_expand(sparse_set *s, arena *a, size_t key) {
    if (sparse_set_is_empty(s)) {
        s->key_range = exclusive_range_singleton(key);
        return LIST_APPEND_EMPTY(dense_index, &s->indices, a, 1);
    } else {
        exclusive_range expanded_range = exclusive_range_from(
            range_union(s->key_range.range, exclusive_range_singleton(key).range)
        );

        range_splitting expansion_ranges = exclusive_range_difference(
            expanded_range,
            s->key_range
        );

        exclusive_range range0 = exclusive_range_from(expansion_ranges.ranges[0]);
        exclusive_range range1 = exclusive_range_from(expansion_ranges.ranges[1]);
        s->key_range = expanded_range;
        if (!exclusive_range_is_empty(range0)) {
            size_t missing_count = exclusive_range_length(range0);
            return LIST_PREPEND_EMPTY(dense_index, &s->indices, a, missing_count);
        } else if (!exclusive_range_is_empty(range1)) {
            size_t missing_count = exclusive_range_length(range1);
            return LIST_APPEND_EMPTY(dense_index, &s->indices, a, missing_count);
        } else {
            assert(false && "unreachable: should not have called expand with a key within bounds");
            return LIST_GET(
                dense_index,
                &s->indices,
                key - s->key_range.start
            );
        }
    }
}

void *sparse_set_set(sparse_set *s, arena *a, size_t key, void *element, size_t element_size) {
    if (exclusive_range_contains(s->key_range, key)) {
        dense_index *index = LIST_GET(dense_index, &s->indices, key - s->key_range.start);
        if (OPTION_IS_SOME(*index)) {
            size_t element_index = OPTION_GET(size_t, *index);
            LIST_SET(size_t, &s->keys, element_index, key);
            return list_set(&s->data, element_index, element, element_size);
        } else {
            *index = OPTION_CREATE_SOME_STRUCT(size_t, list_count_of_size(&s->data, element_size));
            LIST_ADD(size_t, a, &s->keys, key);
            return list_add(&s->data, a, element, element_size);
        }
    } else {
        dense_index *index = sparse_set_expand(s, a, key);
        OPTION_IS_NONE_ASSERT(*index);
        *index = OPTION_CREATE_SOME_STRUCT(size_t, list_count_of_size(&s->data, element_size));
        LIST_ADD(size_t, a, &s->keys, key);
        return list_add(&s->data, a, element, element_size);
    }
}

void sparse_set_remove(sparse_set *s, arena *a, size_t key, optional_element *out_removed_element, size_t element_size) {
    if (!exclusive_range_contains(s->key_range, key)
        || list_count_of_size(&s->data, element_size) == 0) {
        *out_removed_element = OPTION_CREATE_NONE_STRUCT(optional_element);
        return;
    }
    
    dense_index *index = LIST_GET(dense_index, &s->indices, key - s->key_range.start);
    if (OPTION_IS_SOME(*index)) {
        if (OPTION_IS_SOME(*out_removed_element)) {
            memcpy(OPTION_GET(optional_element, *out_removed_element), list_last(&s->data, element_size), element_size);
        }
        size_t removed_index = OPTION_GET(size_t, *index);
        LIST_SET(
            dense_index,
            &s->indices,
            *LIST_REMOVE_SWAP_LAST(size_t, &s->keys, a, removed_index),
            &OPTION_CREATE_SOME_STRUCT(size_t, removed_index)
        );
        list_remove_swap_last(&s->data, a, removed_index, element_size);
        *index = OPTION_CREATE_NONE_STRUCT(size_t);
    } else {
        *out_removed_element = OPTION_CREATE_NONE_STRUCT(optional_element);
    }
}

#endif