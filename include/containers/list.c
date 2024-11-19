#include "list.h"

list list_create() {
    list l;
    l.count = 0;
    l.capacity = 0;
    l.elements = NULL;
    return l;
}

list list_create_with_capacity(arena* a, size_t capacity)
{
    if (capacity == 0)
        return list_create();
    else {
        list l;
        l.count = 0;
        l.capacity = capacity;
        l.elements = arena_alloc(a, capacity);
        return l;
    }
}

void list_grow(list* l, arena* a, size_t new_capacity) {
    assert(new_capacity > l->capacity && "Attempted to grow list to smaller capacity");
    size_t current_capacity = l->capacity;
    if (l->capacity == 0)
        l->capacity = 1;

    while (new_capacity > l->capacity)
        l->capacity *= 2;

    l->elements = arena_realloc(a, l->elements, current_capacity, l->capacity);
}

void list_shrink(list* l, arena* a, size_t new_capacity) {
    assert(new_capacity < l->capacity && "Attempted to shrink list to larger capacity");
    if (new_capacity < l->capacity / 4) {
        size_t current_capacity = l->capacity;
        while (new_capacity < l->capacity / 2)
            l->capacity /= 2;

        l->elements = arena_realloc(a, l->elements, current_capacity, l->capacity);
    }
}

void list_remove(list* l, arena* a, size_t index, size_t size)
{
    assert((index * size < l->count) && "Attempted to remove element with index out of bounds");
    memmove(
        (uint8_t*)l->elements + index * size,
        (uint8_t*)l->elements + (index + 1) * size,
        (l->count - (index + 1) * size));

    size_t new_count = l->count - size;
    if (new_count <= l->capacity / 2)
        list_shrink(l, a, new_count);

    l->count = new_count;
}

void list_remove_range(list* l, arena* a, size_t index, size_t count, size_t size)
{
    assert((index * size < l->count) && "Attempted to remove elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to remove elements with end out of bounds");
    memmove(
        (uint8_t*)l->elements + index * size,
        (uint8_t*)l->elements + (index + count) * size,
        (l->count - (index + count) * size));

    size_t new_count = l->count - count * size;
    if (new_count <= l->capacity / 2)
        list_shrink(l, a, new_count);
    l->count = new_count;
}

void* list_remove_swap_last(list* l, arena* a, size_t index, size_t size) {
    void* swapped = list_set(l, index, list_last(l, size), size);

    size_t new_count = l->count - size;
    if (new_count <= l->capacity / 2)
        list_shrink(l, a, new_count);
    l->count = new_count;
    return swapped;
}

void list_clear(list* l)
{
    l->count = 0;
}

void* list_get_range(const list* l, size_t index, size_t count, size_t size)
{
    assert((index * size < l->count) && "Attempted to get elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to get elements with end out of bounds");
    return (uint8_t*)l->elements + index * size;
}

void* list_set_range(list* l, size_t index, void* elements, size_t count, size_t size) {
    assert((index * size < l->count) && "Attempted to set elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to set elements with end out of bounds");
    uint8_t* destination = (uint8_t*)l->elements + index * size;

    assert(
        (((uint8_t*)elements + count * size <= destination)
            || (destination + count * size <= (uint8_t*)elements))
        && "Should not set range with elements overlapping said range"
    );
    return memcpy((uint8_t*)l->elements + index * size, elements, count * size);
}

void* list_append_empty(list* l, arena* a, size_t count, size_t size) {
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);
    l->count = new_count;
    return list_last(l, size);
}

void* list_prepend_empty(list* l, arena* a, size_t count, size_t size) {
    size_t list_count = l->count;
    list_append_empty(l, a, count, size);
    memmove(
        (uint8_t*)l->elements + count * size,
        (uint8_t*)l->elements,
        list_count
    );
    return l->elements;
}

void* list_insert_range(list* l, arena* a, size_t index, void* elements, size_t count, size_t size) {
    assert((index * size <= l->count) && "Attempted to insert elements with starting index out of bounds");
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);

    memmove(
        (uint8_t*)l->elements + (index + count) * size,
        (uint8_t*)l->elements + index * size,
        (l->count - index * size)
    );
    void* ptr = memcpy((uint8_t*)l->elements + index * size, elements, count * size);
    l->count = new_count;
    return ptr;
}

void* list_insert(list* l, arena* a, size_t index, void* element, size_t size) {
    assert((index * size <= l->count) && "Attempted to insert element with index out of bounds");
    size_t new_count = l->count + size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);

    memmove(
        (uint8_t*)l->elements + (index + 1) * size,
        (uint8_t*)l->elements + index * size,
        (l->count - index * size)
    );
    void* ptr = memcpy((uint8_t*)l->elements + index * size, element, size);
    l->count = new_count;
    return ptr;
}

void* list_get(const list* l, size_t index, size_t size)
{
    assert((index * size < l->count) && "Attempted to get element with index out of bounds");
    return (uint8_t*)l->elements + index * size;
}

void* list_set(list* l, size_t index, void* element, size_t size)
{
    assert((index * size < l->count) && "Attempted to set element with index out of bounds");
    return memcpy((uint8_t*)l->elements + index * size, element, size);
}

void* list_add_range(list* l, arena* a, void* elements, size_t count, size_t size) {
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);

    void* ptr = memcpy((uint8_t*)l->elements + l->count, elements, count * size);
    l->count = new_count;
    return ptr;
}

void* list_add(list* l, arena* a, const void* element, size_t size) {
    size_t new_count = l->count + size;
    if (new_count > l->capacity)
        list_grow(l, a, new_count);

    void* ptr = memcpy((uint8_t*)l->elements + l->count, element, size);
    l->count = new_count;
    return ptr;
}
