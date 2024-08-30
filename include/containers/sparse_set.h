#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include "arena.h"
#include "list.h"

#define SPARSE_SET_INDEX_OFFSET 1

#define SPARSE_SET_INDEX_TYPE size_t
typedef SPARSE_SET_INDEX_TYPE sparse_set_index;

typedef struct sparse_set {
    list indices;
    list data;
    size_t key_of_last_element;
} sparse_set;

inline size_t sparse_set_data_count_of_size(const sparse_set *s, size_t size) {
    return list_count_of_size(&s->data, size);
}
#define SPARSE_SET_DATA_COUNT(type, sparse_set_ref) sparse_set_data_count_of_size(sparse_set_ref, sizeof(type))

inline size_t sparse_set_index_count(const sparse_set *s) {
    return LIST_COUNT(sparse_set_index, &s->indices);
}

sparse_set sparse_set_create(arena *a, size_t capacity) {
    return (sparse_set) {
        .indices = list_create(a, capacity),
        .data = list_create(a, capacity),
        .key_of_last_element = 0
    };
}
#define SPARSE_SET_CREATE(type, arena_ref, capacity) sparse_set_create(arena_ref, (capacity) * sizeof(type))

bool sparse_set_try_get(const sparse_set *s, size_t key, void **out_elelement, size_t size, sparse_set_index *out_index) {
    assert(key < sparse_set_index_count(s) && "Key out of bounds");
    sparse_set_index index = LIST_GET(sparse_set_index, &s->indices, key);
    if (index >= SPARSE_SET_INDEX_OFFSET) {
        *out_elelement = list_get(&s->data, index - SPARSE_SET_INDEX_OFFSET, size);
        *out_index = index;
        return true;
    } else {
        return false;
    }
}

void *sparse_set_insert(sparse_set *s, arena *a, size_t key, void *element, size_t size) {
    void *existing = NULL;
    sparse_set_index existing_index = 0;
    if (key >= sparse_set_index_count(s)) {
        size_t remaining = key - sparse_set_index_count(s);
        size_t *buffer = calloc(remaining, sizeof(size_t));
        LIST_ADD_RANGE(size_t, &s->indices, a, buffer, remaining);
        free(buffer);
    } else if (sparse_set_try_get(s, key, &existing, size, &existing_index)) {
        return list_set(&s->data, existing_index - SPARSE_SET_INDEX_OFFSET, element, size);
    } else {
        
    }
    s->key_of_last_element = key;
    LIST_SET(sparse_set_index, &s->indices, key, sparse_set_data_count_of_size(s, size) + SPARSE_SET_INDEX_OFFSET);
    return list_add(&s->data, a, element, size);
}
#define SPARSE_SET_INSERT(type, sparse_set_ref, arena_ref, key, element_ref) sparse_set_insert(sparse_set_ref, arena_ref, key, element_ref, sizeof(type))


void *sparse_set_get(const sparse_set *s, size_t key, size_t size) {
    assert(key < sparse_set_index_count(s) && "Key out of bounds");
    sparse_set_index index = LIST_GET(sparse_set_index, &s->indices, key);
    assert(index >= SPARSE_SET_INDEX_OFFSET && "Element not inserted");
    return list_get(&s->data, index - SPARSE_SET_INDEX_OFFSET, size);
}
#define SPARSE_SET_GET(type, sparse_set_ref, key) sparse_set_get(sparse_set_ref, key, sizeof(type))

void sparse_set_remove(sparse_set *s, size_t key, size_t size) {
    assert(key < sparse_set_index_count(s) && "Key out of bounds");
    sparse_set_index index = LIST_GET(sparse_set_index, &s->indices, key);
    if (index >= SPARSE_SET_INDEX_OFFSET) {
        list_set(&s->data, index - SPARSE_SET_INDEX_OFFSET, list_last(&s->data, size), size);
    }
}

#endif