#include <memory.h>
#include <assert.h>

#include "cecs_dynamic_array.h"

cecs_dynamic_array cecs_dynamic_array_create(void) {
    cecs_dynamic_array l;
    l.count = 0;
    l.capacity = 0;
    l.elements = NULL;
    return l;
}

cecs_dynamic_array cecs_dynamic_array_create_with_capacity(cecs_arena* a, size_t capacity)
{
    if (capacity == 0)
        return cecs_dynamic_array_create();
    else {
        cecs_dynamic_array l;
        l.count = 0;
        l.capacity = capacity;
        l.elements = cecs_arena_alloc(a, capacity);
        return l;
    }
}

static void cecs_dynamic_array_grow(cecs_dynamic_array* l, cecs_arena* a, size_t new_capacity) {
    assert(new_capacity > l->capacity && "Attempted to grow dynamic array to smaller capacity");
    size_t current_capacity = l->capacity;
    if (l->capacity == 0)
        l->capacity = 1;

    while (new_capacity > l->capacity)
        l->capacity *= 2;

    l->elements = cecs_arena_realloc(a, l->elements, current_capacity, l->capacity);
}

static void cecs_dynamic_array_shrink(cecs_dynamic_array* l, cecs_arena* a, size_t new_capacity) {
    assert(new_capacity < l->capacity && "Attempted to shrink dynamic array to larger capacity");
    if (new_capacity < l->capacity / 4) {
        size_t current_capacity = l->capacity;
        while (new_capacity < l->capacity / 2)
            l->capacity /= 2;

        l->elements = cecs_arena_realloc(a, l->elements, current_capacity, l->capacity);
    }
}

void cecs_dynamic_array_remove(cecs_dynamic_array* l, cecs_arena* a, size_t index, size_t size)
{
    assert((index * size < l->count) && "Attempted to remove element with index out of bounds");
    memmove(
        (uint8_t*)l->elements + index * size,
        (uint8_t*)l->elements + (index + 1) * size,
        (l->count - (index + 1) * size));

    size_t new_count = l->count - size;
    if (new_count <= l->capacity / 2)
        cecs_dynamic_array_shrink(l, a, new_count);

    l->count = new_count;
}

void cecs_dynamic_array_remove_range(cecs_dynamic_array* l, cecs_arena* a, size_t index, size_t count, size_t size)
{
    assert((index * size < l->count) && "Attempted to remove elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to remove elements with end out of bounds");
    memmove(
        (uint8_t*)l->elements + index * size,
        (uint8_t*)l->elements + (index + count) * size,
        (l->count - (index + count) * size));

    size_t new_count = l->count - count * size;
    if (new_count <= l->capacity / 2)
        cecs_dynamic_array_shrink(l, a, new_count);
    l->count = new_count;
}

void* cecs_dynamic_array_remove_swap_last(cecs_dynamic_array* l, cecs_arena* a, size_t index, size_t size) {
    void* swapped = cecs_dynamic_array_set(l, index, cecs_dynamic_array_last(l, size), size);

    size_t new_count = l->count - size;
    if (new_count <= l->capacity / 2)
        cecs_dynamic_array_shrink(l, a, new_count);
    l->count = new_count;
    return swapped;
}

void cecs_dynamic_array_clear(cecs_dynamic_array* l)
{
    l->count = 0;
}

void* cecs_dynamic_array_get_range(const cecs_dynamic_array* l, size_t index, size_t count, size_t size)
{
    assert((index * size < l->count) && "Attempted to get elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to get elements with end out of bounds");
    return (uint8_t*)l->elements + index * size;
}

void* cecs_dynamic_array_set_range(cecs_dynamic_array* l, size_t index, void* elements, size_t count, size_t size) {
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

void *cecs_dynamic_array_set_copy_range(cecs_dynamic_array *l, size_t index, void *single_src, size_t count, size_t size) {
    assert((index * size < l->count) && "Attempted to set elements with starting index out of bounds");
    assert(((index + count) * size <= l->count) && "Attempted to set elements with end out of bounds");
    uint8_t* destination = (uint8_t*)l->elements + index * size;

    assert(
        (((uint8_t*)single_src + size <= destination)
            || (destination + count * size <= (uint8_t*)single_src))
        && "Should not set range with elements overlapping said range"
    );
    memcpy(destination, single_src, size);

    size_t copied = size;
    size_t total = count * size;
    while ((copied << 1) < total) {
        memcpy(destination + copied, destination, copied);
        copied <<= 1;
    }
    assert(copied >= total / 2 && "error: copied bytes should be at least half of total bytes");
    assert(copied <= total && "error: copied bytes should be less than or equal to total bytes");
    
    memcpy(destination + copied, destination, total - copied);
    return destination;
}

void* cecs_dynamic_array_append_empty(cecs_dynamic_array* l, cecs_arena* a, size_t count, size_t size) {
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        cecs_dynamic_array_grow(l, a, new_count);
    l->count = new_count;
    return cecs_dynamic_array_last(l, size);
}

void* cecs_dynamic_array_prepend_empty(cecs_dynamic_array* l, cecs_arena* a, size_t count, size_t size) {
    size_t cecs_dynamic_array_count = l->count;
    cecs_dynamic_array_append_empty(l, a, count, size);
    memmove(
        (uint8_t*)l->elements + count * size,
        (uint8_t*)l->elements,
        cecs_dynamic_array_count
    );
    return l->elements;
}

void* cecs_dynamic_array_insert_range(cecs_dynamic_array* l, cecs_arena* a, size_t index, void* elements, size_t count, size_t size) {
    assert((index * size <= l->count) && "Attempted to insert elements with starting index out of bounds");
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        cecs_dynamic_array_grow(l, a, new_count);

    memmove(
        (uint8_t*)l->elements + (index + count) * size,
        (uint8_t*)l->elements + index * size,
        (l->count - index * size)
    );
    void* ptr = memcpy((uint8_t*)l->elements + index * size, elements, count * size);
    l->count = new_count;
    return ptr;
}

void* cecs_dynamic_array_insert(cecs_dynamic_array* l, cecs_arena* a, size_t index, void* element, size_t size) {
    assert((index * size <= l->count) && "Attempted to insert element with index out of bounds");
    size_t new_count = l->count + size;
    if (new_count > l->capacity)
        cecs_dynamic_array_grow(l, a, new_count);

    memmove(
        (uint8_t*)l->elements + (index + 1) * size,
        (uint8_t*)l->elements + index * size,
        (l->count - index * size)
    );
    void* ptr = memcpy((uint8_t*)l->elements + index * size, element, size);
    l->count = new_count;
    return ptr;
}

void* cecs_dynamic_array_get(const cecs_dynamic_array* l, size_t index, size_t size)
{
    assert((index * size < l->count) && "Attempted to get element with index out of bounds");
    return (uint8_t*)l->elements + index * size;
}

void* cecs_dynamic_array_set(cecs_dynamic_array* l, size_t index, void* element, size_t size)
{
    assert((index * size < l->count) && "Attempted to set element with index out of bounds");
    return memcpy((uint8_t*)l->elements + index * size, element, size);
}

void* cecs_dynamic_array_add_range(cecs_dynamic_array* l, cecs_arena* a, void* elements, size_t count, size_t size) {
    size_t new_count = l->count + count * size;
    if (new_count > l->capacity)
        cecs_dynamic_array_grow(l, a, new_count);

    void* ptr = memcpy((uint8_t*)l->elements + l->count, elements, count * size);
    l->count = new_count;
    return ptr;
}

void* cecs_dynamic_array_add(cecs_dynamic_array* l, cecs_arena* a, const void* element, size_t size) {
    size_t new_count = l->count + size;
    if (new_count > l->capacity)
        cecs_dynamic_array_grow(l, a, new_count);

    void* ptr = memcpy((uint8_t*)l->elements + l->count, element, size);
    l->count = new_count;
    return ptr;
}
