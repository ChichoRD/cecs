#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "tagged_union.h"
#include "range.h"
#include "arena.h"
#include "list.h"

typedef OPTION_STRUCT(size_t, dense_index) dense_index;
typedef OPTION_STRUCT(void *, optional_element) optional_element;

typedef list any_elements;
typedef list integer_elements;
typedef TAGGED_UNION_STRUCT(sparse_set_elements, list, any_elements, list, integer_elements) sparse_set_elements;
typedef struct sparse_set {
    sparse_set_elements elements;
    list indices;
    list keys;
    exclusive_range key_range;
} sparse_set;

sparse_set sparse_set_create(arena *a, size_t element_capacity, size_t element_size) {
    return (sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(any_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
        .indices = list_create_with_capacity(a, sizeof(dense_index) * element_capacity),
        .keys = list_create_with_capacity(a, sizeof(size_t) * element_capacity),
        .key_range = (exclusive_range) { 0, 0 }
    };
}

sparse_set sparse_set_create_of_integers(arena *a, size_t element_capacity, size_t element_size) {
    return (sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(integer_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
        .indices = list_create_with_capacity(a, sizeof(dense_index) * element_capacity),
        .keys = list_create(),
        .key_range = (exclusive_range) { 0, 0 }
    };
}

inline size_t sparse_set_count_of_size(const sparse_set *s, size_t element_size) {
    return list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size);
}

inline bool sparse_set_contains(const sparse_set *s, size_t key) {
    return exclusive_range_contains(s->key_range, key)
        && OPTION_IS_SOME(dense_index, *LIST_GET(dense_index, &s->indices, key - s->key_range.start));
}

optional_element sparse_set_get(const sparse_set *s, size_t key, size_t element_size) {
    if (!exclusive_range_contains(s->key_range, key)) {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }

    size_t list_index = key - s->key_range.start;
    dense_index index = *LIST_GET(dense_index, &s->indices, list_index);
    if (OPTION_IS_SOME(dense_index, index)) {
        return OPTION_CREATE_SOME_STRUCT(
            optional_element, 
            list_get(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), OPTION_GET(dense_index, index), element_size)
        );
    } else {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }
}
#define SPARSE_SET_GET(type, sparse_set_ref, key) \
    sparse_set_get(sparse_set_ref, key, sizeof(type))

void *sparse_set_get_unchecked(const sparse_set *s, size_t key, size_t element_size) {
    return OPTION_GET(optional_element, sparse_set_get(s, key, element_size));
}
#define SPARSE_SET_GET_UNCHECKED(type, sparse_set_ref, key) \
    ((type *)sparse_set_get_unchecked(sparse_set_ref, key, sizeof(type)))

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
        if (OPTION_IS_SOME(dense_index, *index)) {
            size_t element_index = OPTION_GET(dense_index, *index);
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_SET(size_t, &s->keys, element_index, &key);
            return list_set(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_index, element, element_size);
        } else {
            *index = OPTION_CREATE_SOME_STRUCT(dense_index, list_count_of_size(&s->elements, element_size));
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_ADD(dense_index, &s->keys, a, &key);
            return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
        }
    } else {
        dense_index *index = sparse_set_expand(s, a, key);
        OPTION_IS_NONE_ASSERT(dense_index, *index);
        *index = OPTION_CREATE_SOME_STRUCT(
            dense_index,
            list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size)
        );
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
            LIST_ADD(size_t, &s->keys, a, &key);
        return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
    }
}
#define SPARSE_SET_SET(type, sparse_set_ref, arena_ref, key, element_ref) \
    sparse_set_set(sparse_set_ref, arena_ref, key, element_ref, sizeof(type))

void sparse_set_remove(sparse_set *s, arena *a, size_t key, optional_element *out_removed_element, size_t element_size) {
    if (!exclusive_range_contains(s->key_range, key)
        || list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size) == 0) {
        *out_removed_element = OPTION_CREATE_NONE_STRUCT(optional_element);
        return;
    }
    
    dense_index *index = LIST_GET(dense_index, &s->indices, key - s->key_range.start);
    if (OPTION_IS_SOME(dense_index, *index)) {
        if (OPTION_IS_SOME(optional_element, *out_removed_element)) {
            memcpy(
                OPTION_GET(optional_element, *out_removed_element),
                list_last(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size),
                element_size
            );
        }
        size_t removed_index = OPTION_GET(dense_index, *index);
        void *swapped_last = list_remove_swap_last(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, removed_index, element_size);
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements)) {
            LIST_SET(
                dense_index,
                &s->indices,
                *LIST_REMOVE_SWAP_LAST(size_t, &s->keys, a, removed_index),
                &OPTION_CREATE_SOME_STRUCT(dense_index, removed_index)
            );
        } else {
            LIST_SET(
                dense_index,
                &s->indices,
                *((size_t *)swapped_last),
                &OPTION_CREATE_SOME_STRUCT(dense_index, removed_index)
            );
        }
        *index = OPTION_CREATE_NONE_STRUCT(dense_index);
    } else {
        *out_removed_element = OPTION_CREATE_NONE_STRUCT(optional_element);
    }
}
#define SPARSE_SET_REMOVE(type, sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    sparse_set_remove(sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))

void sparse_set_remove_unchecked(sparse_set *s, arena *a, size_t key, void *out_removed_element, size_t element_size) {
    sparse_set_remove(s, a, key, &OPTION_CREATE_SOME_STRUCT(optional_element, out_removed_element), element_size);
}
#define SPARSE_SET_REMOVE_UNCHECKED(type, sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    sparse_set_remove_unchecked(sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))


#endif