#ifndef LIST_H
#define LIST_H

#include <memory.h>
#include <stddef.h>
#include "arena.h"

#define padding_between(type, next_type) (offsetof(struct { type _a; next_type _b; }, _b) - sizeof(type))

typedef struct list
{
    size_t count;
    size_t capacity;
    void *elements;
} list;

inline size_t list_count_of_size(const list *l, size_t size)
{
    return l->count / size;
}

inline size_t list_capacity_of_size(const list *l, size_t size)
{
    return l->capacity / size;
}

#define LIST_COUNT(type, list_ref) list_count_of_size(list_ref, sizeof(type))
#define LIST_CAPACITY(type, list_ref) list_capacity_of_size(list_ref, sizeof(type))

#define LIST_FOREACH(type, list_ref, element) \
    for (type *element = (type *)(list_ref)->elements; element < (type *)(list_ref)->elements + (list_ref).count; element++)

list list_create(arena *a, size_t capacity)
{
    list l;
    l.count = 0;
    l.capacity = capacity;
    l.elements = arena_alloc(a, capacity);
    return l;
}
#define LIST_CREATE(type, arena_ref, capacity) list_create(arena_ref, (capacity) * sizeof(type))

static void list_grow(list *l, arena *a, size_t new_capacity) {
    assert(new_capacity > l->capacity && "Attempted to grow list to smaller capacity");
    size_t current_capacity = l->capacity;
    while (new_capacity > l->capacity)
        l->capacity *= 2;

    l->elements = arena_realloc(a, l->elements, current_capacity, l->capacity);
}

static void list_shrink(list *l, arena *a, size_t new_capacity) {
    assert(new_capacity < l->capacity && "Attempted to shrink list to larger capacity");
    size_t current_capacity = l->capacity;
    while (new_capacity < l->capacity)
        l->capacity /= 2;

    l->elements = arena_realloc(a, l->elements, current_capacity, l->capacity);
}

void *list_add(list *l, arena *a, const void *element, size_t size)
{
    assert(l->elements != NULL && "Attempted to add element to list without initializing it first");
    size_t new_count = l->count + size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);

    void *ptr = memcpy((uint8_t *)l->elements + l->count, element, size);
    l->count = new_count;
    return ptr;
}
#define LIST_ADD(type, list_ref, arena_ref, element_ref) \
    ((type *)list_add(list_ref, arena_ref, element_ref, sizeof(type)))

void *list_add_range(list *l, arena *a, void *elements, size_t count, size_t size)
{
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);

    void *ptr = memcpy((uint8_t *)l->elements + l->count, elements, count * size);
    l->count = new_count;
    return ptr;
}
#define LIST_ADD_RANGE(type, list_ref, arena_ref, elements_ref, count) \
    list_add_range(list_ref, arena_ref, elements_ref, count, sizeof(type))

void list_remove(list *l, arena *a, size_t index, size_t size)
{
    assert((index * size < l->count) && "Attempted to remove element with index out of bounds");
    memmove(
        (uint8_t *)l->elements + index * size,
        (uint8_t *)l->elements + (index + 1) * size,
        (l->count - (index + 1) * size));

    size_t new_count = l->count - size;
    if (new_count <= l->capacity / 2)
        list_shrink(l, a, new_count);

    l->count = new_count;
}
#define LIST_REMOVE(type, list_ref, arena_ref, index) \
    list_remove(list_ref, arena_ref, index, sizeof(type))

void list_remove_range(list *l, arena *a, size_t index, size_t count, size_t size)
{
    assert((index * size < l->count) && "Attempted to remove elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to remove elements with end out of bounds");
    memmove(
        (uint8_t *)l->elements + index * size,
        (uint8_t *)l->elements + (index + count) * size,
        (l->count - (index + count) * size));

    size_t new_count = l->count - count * size;
    if (new_count <= l->capacity / 2)
        list_shrink(l, a, new_count);
    l->count = new_count;
}
#define LIST_REMOVE_RANGE(type, lis_ref, arena_ref, index, count) list_remove_range(lis_ref, arena_ref, index, count, sizeof(type))

void *list_set(list *l, size_t index, void *element, size_t size)
{
    assert((index * size < l->count) && "Attempted to set element with index out of bounds");
    return memcpy((uint8_t *)l->elements + index * size, element, size);
}
#define LIST_SET(type, lis_ref, index, element_ref) \
    ((type *)list_set(lis_ref, index, element_ref, sizeof(type)))

inline void *list_last(const list *l, size_t size) {
    assert(l->count > 0 && "Attempted to get last element of empty list");
    return (uint8_t *)l->elements + (l->count - size);
}
#define LIST_LAST(type, lis_ref) ((type *)list_last(lis_ref, sizeof(type)))

void list_remove_swap_last(list *l, arena *a, size_t index, size_t size) {
    list_set(l, index, list_last(l, size), size);

    size_t new_count = l->count - size;
    if (new_count <= l->capacity / 2)
        list_shrink(l, a, new_count);
    l->count = new_count;
}
#define LIST_REMOVE_SWAP_LAST(type, lis_ref, arena_ref, index) list_remove_swap_last(lis_ref, arena_ref, index, sizeof(type))

void list_clear(list *l)
{
    l->count = 0;
}

void *list_get(const list *l, size_t index, size_t size)
{
    assert((index * size < l->count) && "Attempted to get element with index out of bounds");
    return (uint8_t *)l->elements + index * size;
}
#define LIST_GET(type, lis_ref, index) ((type *)list_get(lis_ref, index, sizeof(type)))

void *list_get_range(const list *l, size_t index, size_t count, size_t size)
{
    assert((index * size < l->count) && "Attempted to get elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to get elements with end out of bounds");
    return (uint8_t *)l->elements + index * size;
}
#define LIST_GET_RANGE(type, lis_ref, index, count) \
    ((type *)list_get_range(lis_ref, index, count, sizeof(type)))

void *list_set_range(list *l, size_t index, void *elements, size_t count, size_t size) {
    assert((index * size < l->count) && "Attempted to set elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to set elements with end out of bounds");
    return memcpy((uint8_t *)l->elements + index * size, elements, count * size);
}
#define LIST_SET_RANGE(type, lis_ref, index, elements_ref, count) \
    list_set_range(lis_ref, index, elements_ref, count, sizeof(type))

void *list_insert(list *l, arena *a, size_t index, void *element, size_t size)
{
    assert((index * size <= l->count) && "Attempted to insert element with index out of bounds");
    size_t new_count = l->count + size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);

    memmove(
        (uint8_t *)l->elements + (index + 1) * size,
        (uint8_t *)l->elements + index * size,
        (l->count - index * size));
    void *ptr = memcpy((uint8_t *)l->elements + index * size, element, size);
    l->count = new_count;
    return ptr;
}
#define LIST_INSERT(type, lis_ref, arena_ref, index, element_ref) \
    ((type *)list_insert(lis_ref, arena_ref, index, element_ref, sizeof(type)))

void *list_insert_range(list *l, arena *a, size_t index, void *elements, size_t count, size_t size)
{
    assert((index * size <= l->count) && "Attempted to insert elements with starting index out of bounds");
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);

    memmove(
        (uint8_t *)l->elements + (index + count) * size,
        (uint8_t *)l->elements + index * size,
        (l->count - index * size));
    void *ptr = memcpy((uint8_t *)l->elements + index * size, elements, count * size);
    l->count = new_count;
    return ptr;
}
#define LIST_INSERT_RANGE(type, lis_ref, arena_ref, index, elements_ref, count) \
    list_insert_range(lis_ref, arena_ref, index, elements_ref, count, sizeof(type))

void *list_append_empty(list *l, arena *a, size_t count, size_t size) {
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);
    l->count = new_count;
    return list_last(l, size);
}
#define LIST_APPEND_EMPTY(type, lis_ref, arena_ref, count) \
    ((type *)list_append_empty(lis_ref, arena_ref, count, sizeof(type)))

void *list_prepend_empty(list *l, arena *a, size_t count, size_t size) {
    list_insert_range(l, a, count, l->elements, l->count, size);
    return l->elements;
}
#define LIST_PREPEND_EMPTY(type, lis_ref, arena_ref, count) \
    ((type *)list_prepend_empty(lis_ref, arena_ref, count, sizeof(type)))

#endif