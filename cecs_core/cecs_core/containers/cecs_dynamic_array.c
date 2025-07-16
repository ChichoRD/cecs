#include <memory.h>
#include <assert.h>

#include "cecs_dynamic_array.h"
#include <cecs_math/relations/cecs_ordering.h>

cecs_dynarray cecs_dynarray_create_with_capacity(cecs_allocator* a, const size_t value_size, const size_t values_capacity) {
    if (value_size == 0) {
        assert(false && "error: attempted to create dynamic array with zero size elements");
        exit(EXIT_FAILURE);
    } else if (values_capacity == 0) {
        return cecs_dynarray_create();
    } else {
        return (cecs_dynarray) {
            .values = cecs_allocator_alloc_aligned(a,  value_size * values_capacity, cecs_max_alignment_from_size(value_size)),
            .values_used = 0,
            .values_capacity = values_capacity
        };
    }
}

void cecs_dynarray_reserve_exact(cecs_dynarray* arr, cecs_allocator* a, const size_t value_size, const size_t values_new_capacity) {
    if (values_new_capacity > arr->values_capacity) {
        arr->values = cecs_allocator_realloc_aligned(
            a,
            arr->values,
            value_size * arr->values_capacity,
            value_size * values_new_capacity,
            cecs_max_alignment_from_size(value_size)
        );
        arr->values_capacity = values_new_capacity;
    } else if (values_new_capacity < arr->values_capacity) {
        assert(false && "fatal error: attempted to grow dynamic array to smaller capacity");
        exit(EXIT_FAILURE);
    }
}
void cecs_dynarray_reserve(cecs_dynarray* arr, cecs_allocator* a, const size_t value_size, const size_t values_new_capacity) {
    cecs_dynarray_reserve_exact(arr, a, value_size, cecs_max(values_new_capacity, arr->values_capacity << 1));
}

void cecs_dynarray_shrink_exact(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size, const size_t values_new_capacity) {
    if (values_new_capacity < arr->values_used) {
        assert(
            false
            && "fatal error: attempted to shrink dynamic array to smaller capacity than used."
            "Use cecs_dynarray_truncate() to truncate the array before shrinking."
        );
        exit(EXIT_FAILURE);
    } else if (values_new_capacity < arr->values_capacity) {
        arr->values = cecs_allocator_realloc_aligned(
            a,
            arr->values,
            value_size * arr->values_capacity,
            value_size * values_new_capacity,
            cecs_max_alignment_from_size(value_size)
        );
        arr->values_capacity = values_new_capacity;
    } else if (values_new_capacity > arr->values_capacity) {
        assert(false && "fatal error: attempted to shrink dynamic array to a capacity larger or equal than current capacity");
        exit(EXIT_FAILURE);
    }
}
void cecs_dynarray_shrink(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size, const size_t values_new_capacity) {
    cecs_dynarray_shink_exact(arr, a, value_size, cecs_max(values_new_capacity, arr->values_used));
}

void *cecs_dynarray_push(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size) {
    if (arr->values_used + 1 > arr->values_capacity) {
        cecs_dynarray_reserve_exact(arr, a, value_size, arr->values_capacity << 1);
    }
    void *const element = arr->values + arr->values_used * value_size;
    ++arr->values_used;
    return element;
}
void *cecs_dynarray_push_many(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size, const size_t count) {
    const size_t new_count = arr->values_used + count;
    if (new_count > arr->values_capacity) {
        cecs_dynarray_reserve(arr, a, value_size, new_count);
    }
    void *const elements = arr->values + arr->values_used * value_size;
    arr->values_used = new_count;
    return elements;
}

void *cecs_dynarray_push_many_cpy(cecs_dynarray *arr, cecs_allocator *a, const void *values, const size_t value_size, const size_t count) {
    void *const elements = cecs_dynarray_push_many(arr, a, value_size, count);
    return memcpy(elements, values, count * value_size);
}

void *cecs_dynarray_extend(cecs_dynarray *arr, cecs_allocator *a, const size_t start_index_inclusive, const size_t end_index_exclusive, const size_t value_size) {
    if (start_index_inclusive >= arr->values_used) {
        assert(false && "error: attempted to extend dynamic array with starting index out of bounds");
        exit(EXIT_FAILURE);
    } else if (end_index_exclusive > arr->values_used) {
        assert(false && "error: attempted to extend dynamic array with end index out of bounds");
        exit(EXIT_FAILURE);
    }

    const size_t values_count = end_index_exclusive - start_index_inclusive;
    const size_t new_count = arr->values_used + values_count;
    void *const elements = arr->values + start_index_inclusive * value_size;
    if (values_count == 0) {
        return elements;
    } else if (new_count > arr->values_capacity) {
        cecs_dynarray_reserve(arr, a, value_size, new_count);
    }

    memmove(
        arr->values + arr->values_used * value_size,
        elements,
        values_count * value_size
    );
    arr->values_used = new_count;
    return elements;
}
void *cecs_dynarray_insert_many_cpy(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const void *values, const size_t value_size, const size_t count) {
    void *const elements = cecs_dynarray_insert_many(arr, a, index, value_size, count);
    return memcpy(elements, values, count * value_size);
}

void cecs_dynarray_pop(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size)
{
    --arr->values_used;
    if (arr->values_used < (arr->values_capacity >> 1)) {
        cecs_dynarray_shrink_exact(arr, a, value_size, arr->values_capacity >> 1);
    }
}
void cecs_dynarray_truncate(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size, const size_t new_count) {
    arr->values_used = new_count;
    if (arr->values_used < (arr->values_capacity >> 1)) {
        cecs_dynarray_shrink_exact(arr, a, value_size, arr->values_capacity >> 1);
    }
}
void cecs_dynarray_swap_last_pop(cecs_dynarray* arr, cecs_allocator* a, const size_t index, const size_t value_size) {
    switch (arr->values_used) {
    case 0: {
        assert(false && "error: attempted to swap last element of empty cecs_dynarray");
        exit(EXIT_FAILURE);
    }
    case 1: {
        --arr->values_used;
        return;
    }
    default: {
        void *const swapped = cecs_dynarray_get_mut(arr, index, value_size);
        const void *const last = cecs_dynarray_last(arr, value_size);
        memcpy(swapped, last, value_size);
    }
    }
}

void cecs_dynarray_remove(cecs_dynarray* arr, cecs_allocator* a, const size_t index, const size_t size) {
    if (index >= arr->values_used) {
        assert(false && "error: attempted to remove element with index out of bounds");
        exit(EXIT_FAILURE);
    }

    --arr->values_used;
    memmove(
        arr->values + index * size,
        arr->values + (index + 1) * size,
        (arr->values_used - index) * size
    );

    const size_t half_capacity = arr->values_capacity >> 1;
    if (arr->values_used < half_capacity) {
        cecs_dynarray_shrink_exact(arr, a, size, half_capacity);
    }
}
void cecs_dynarray_remove_many(cecs_dynarray* arr, cecs_allocator* a, const size_t index, const size_t count, const size_t size) {
    if (index >= arr->values_used) {
        assert(false && "error: attempted to remove elements with starting index out of bounds");
        exit(EXIT_FAILURE);
    } else if (index + count > arr->values_used) {
        assert(false && "error: attempted to remove elements with end out of bounds");
        exit(EXIT_FAILURE);
    }

    arr->values_used -= count;
    memmove(
        arr->values + index * size,
        arr->values + (index + count) * size,
        (arr->values_used - index) * size
    );

    const size_t half_capacity = arr->values_capacity >> 1;
    if (arr->values_used < half_capacity) {
        cecs_dynarray_shrink_exact(arr, a, size, half_capacity);
    }
}

static inline void *cecs_dynarray_get_ptr(const cecs_dynarray *arr, const size_t index, const size_t size) {
    if (index >= arr->values_used) {
        assert(false && "error: attempted to get element with index out of bounds");
        exit(EXIT_FAILURE);
    }
    return arr->values + index * size;
}
void* cecs_dynarray_get_mut(cecs_dynarray* arr, const size_t index, const size_t size) {
    return cecs_dynarray_get_ptr(arr, index, size);
}
const void *cecs_dynarray_get(const cecs_dynarray *arr, const size_t index, const size_t size) {
    return cecs_dynarray_get_ptr(arr, index, size);
}
static inline void *cecs_dynarray_get_range_ptr(const cecs_dynarray *arr, const size_t index, const size_t count, const size_t size) {
    if (index >= arr->values_used) {
        assert(false && "error: attempted to get elements with starting index out of bounds");
        exit(EXIT_FAILURE);
    } else if (index + count > arr->values_used) {
        assert(false && "error: attempted to get elements with end out of bounds");
        exit(EXIT_FAILURE);
    }
    return arr->values + index * size;
}
void *cecs_dynarray_get_range_mut(cecs_dynarray* arr, const size_t index, const size_t count, const size_t size) {
    return cecs_dynarray_get_range_ptr(arr, index, count, size);
}
const void *cecs_dynarray_get_range(const cecs_dynarray *arr, const size_t index, const size_t count, const size_t size) {
    return cecs_dynarray_get_range_ptr(arr, index, count, size);
}
