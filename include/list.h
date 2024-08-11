#ifndef LIST_H
#define LIST_H

#include <memory.h>
#include "arena.h"

typedef struct list
{
    size_t count;
    size_t capacity;
    void *elements;
} list;

#define LIST_COUNT_OF_SIZE(list0, size) ((list0).count / size)
#define LIST_CAPACITY_OF_SIZE(list0, size) ((list0).capacity / size)

#define LIST_COUNT(type, list0) LIST_COUNT_OF_SIZE(list0, sizeof(type))
#define LIST_CAPACITY(type, list0) LIST_CAPACITY_OF_SIZE(list0, sizeof(type))

#define LIST_FOREACH(type, list0, element) for (type *element = (type *)(list0).elements; element < (type *)(list0).elements + (list0).count; element++)

list list_create(arena *a, size_t capacity)
{
    list l;
    l.count = 0;
    l.capacity = capacity;
    l.elements = arena_alloc(a, capacity);
    return l;
}
#define LIST_CREATE(type, arena0, capacity) list_create(arena0, capacity * sizeof(type))

void *list_add(list *l, arena *a, void *element, size_t size)
{
    size_t new_count = l->count + size;
    if (new_count > l->capacity)
    {
        while (new_count > l->capacity)
            l->capacity *= 2;
        list new = list_create(a, l->capacity);
        new.count = l->count;

        memcpy(new.elements, l->elements, l->count);
        l = &new;
    }

    void *ptr = memcpy((uint8_t *)l->elements + l->count, element, size);
    l->count = new_count;
    return ptr;
}
#define LIST_ADD(type, list0, arena0, element) *(type *)list_add(list0, arena0, &element, sizeof(type))

void *list_add_range(list *l, arena *a, void *elements, size_t count, size_t size)
{
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
    {
        while (new_count > l->capacity)
            l->capacity *= 2;
        list new = list_create(a, l->capacity);
        new.count = l->count;

        memcpy(new.elements, l->elements, l->count);
        l = &new;
    }

    void *ptr = memcpy((uint8_t *)l->elements + l->count, elements, count * size);
    l->count = new_count;
    return ptr;
}
#define LIST_ADD_RANGE(type, list0, arena0, elements, count) list_add_range(list0, arena0, elements, count, sizeof(type))

void list_remove(list *l, size_t index, size_t size)
{
    assert(index * size < l->count);
    memmove(
        (uint8_t *)l->elements + index * size,
        (uint8_t *)l->elements + (index + 1) * size,
        ((l->count - index - 1) * size));
    l->count -= size;
}
#define LIST_REMOVE(type, list0, index) list_remove(list0, index, sizeof(type))

void list_remove_range(list *l, size_t index, size_t count, size_t size)
{
    assert(index * size < l->count);
    assert((index + count) * size <= l->count);
    memmove(
        (uint8_t *)l->elements + index * size,
        (uint8_t *)l->elements + (index + count) * size,
        ((l->count - index - count) * size));
    l->count -= count * size;
}
#define LIST_REMOVE_RANGE(type, list0, index, count) list_remove_range(list0, index, count, sizeof(type))

void *list_get(const list *l, size_t index, size_t size)
{
    assert(index * size < l->count);
    return (uint8_t *)l->elements + index * size;
}
#define LIST_GET(type, list0, index) *(type *)list_get(list0, index, sizeof(type))

void *list_set(list *l, size_t index, void *element, size_t size)
{
    assert(index * size < l->count);
    return memcpy((uint8_t *)l->elements + index * size, element, size);
}
#define LIST_SET(type, list0, index, element) *(type *)list_set(list0, index, &element, sizeof(type))

void *list_insert(list *l, arena *a, size_t index, void *element, size_t size)
{
    assert(index * size < l->count);
    size_t new_count = l->count + size;
    if (new_count > l->capacity)
    {
        while (new_count > l->capacity)
            l->capacity *= 2;
        list new = list_create(a, l->capacity);
        new.count = l->count;

        memcpy(new.elements, l->elements, l->count);
        l = &new;
    }

    memmove(
        (uint8_t *)l->elements + (index + 1) * size,
        (uint8_t *)l->elements + index * size,
        (l->count - index) * size);
    void *ptr = memcpy((uint8_t *)l->elements + index * size, element, size);
    l->count = new_count;
    return ptr;
}
#define LIST_INSERT(type, list0, arena0, index, element) *(type *)list_insert(list0, arena0, index, &element, sizeof(type))

void *list_insert_range(list *l, arena *a, size_t index, void *elements, size_t count, size_t size)
{
    assert(index * size < l->count);
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
    {
        while (new_count > l->capacity)
            l->capacity *= 2;
        list new = list_create(a, l->capacity);
        new.count = l->count;

        memcpy(new.elements, l->elements, l->count);
        l = &new;
    }

    memmove(
        (uint8_t *)l->elements + (index + count) * size,
        (uint8_t *)l->elements + index * size,
        (l->count - index) * size);
    void *ptr = memcpy((uint8_t *)l->elements + index * size, elements, count * size);
    l->count = new_count;
    return ptr;
}
#define LIST_INSERT_RANGE(type, list0, arena0, index, elements, count) list_insert_range(list0, arena0, index, elements, count, sizeof(type))

#endif