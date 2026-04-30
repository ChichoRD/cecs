#include <memory.h>
#include <assert.h>

#include "cecs_dynarray.h"
#include <cecs_error.h>
#include <relations/cecs_ordering.h>
#include <string.h>

extern inline size_t cecs_array_count(const cecs_array *arr);
extern inline size_t cecs_array_capacity(const cecs_array *arr);
extern inline cecs_array cecs_array_create(void *const values, const size_t capacity);

extern inline size_t cecs_dynarray_count(const cecs_dynarray *arr);
extern inline size_t cecs_dynarray_capacity(const cecs_dynarray *arr);
extern inline cecs_dynarray cecs_dynarray_create(void);

void cecs_array_reserve_exact(cecs_array *arr, const size_t additional_capacity) {
    const size_t current_capacity = cecs_array_capacity(arr);
    const size_t requested_capacity = cecs_array_count(arr) + additional_capacity;
    cecs_debugbreak_fail_if(
        current_capacity < requested_capacity,
        "error: array could not reserve enough capacity for requested additional capacity"
    );
}

void *cecs_array_push(cecs_array *arr, const size_t value_size) {
    cecs_array_reserve_exact(arr, 1ull);
    void *const element = arr->values + arr->values_used * value_size;
    ++arr->values_used;
    return element;
}
void *cecs_array_push_many(cecs_array *arr, const size_t count, const size_t value_size) {
    cecs_array_reserve_exact(arr, count);
    void *const elements = arr->values + arr->values_used * value_size;
    arr->values_used += count;
    return elements;
}
void *cecs_array_push_many_copy(cecs_array *arr, const void *values, const size_t count, const size_t value_size) {
    void *const elements = cecs_array_push_many(arr, count, value_size);
    return memcpy(elements, values, count * value_size);
}

void *cecs_array_extend(cecs_array *arr, const size_t start_index_inclusive, const size_t end_index_exclusive, const size_t value_size) {
    const size_t extend_count = end_index_exclusive - start_index_inclusive;
    if (cecs_expect_not(start_index_inclusive > end_index_exclusive)) {
        cecs_debugbreak_fail_message("error: attempted to extend cecs_array with start index greater than end index");
    } else if (cecs_expect_not(end_index_exclusive > arr->values_used)) {
        cecs_debugbreak_fail_message("error: attempted to extend cecs_array with end index greater than count");
    } else if (cecs_expect_not(arr->values_used + extend_count > arr->values_capacity)) {
        cecs_debugbreak_fail_message("error: attempted to extend cecs_array exceeding its capacity");
    }
    
    void *const extension_destination = arr->values + arr->values_used * value_size;
    if (extend_count > 0) {
        const void *const extension_source = arr->values + start_index_inclusive * value_size;
        memcpy(
            extension_destination,
            extension_source,
            extend_count * value_size
        );
        arr->values_used += extend_count;
    }
    return extension_destination;
}
void *cecs_array_insert(cecs_array *arr, const size_t index, const size_t value_size) {
    if (cecs_expect_not(index > arr->values_used)) {
        cecs_debugbreak_fail_message("error: attempted to insert in cecs_array with end index greater than count");
    } else if (cecs_expect_not(arr->values_used + 1 > arr->values_capacity)) {
        cecs_debugbreak_fail_message("error: attempted to insert in cecs_array exceeding its capacity");
    }

    unsigned char *const insertion_start = arr->values + index * value_size;
    unsigned char *const insertion_end = insertion_start + value_size;
    const size_t move_count = arr->values_used - index;
    const size_t move_size = move_count * value_size;
    if (move_count <= 1) {
        memcpy(insertion_end, insertion_start, move_size);
    } else {
        memmove(insertion_end, insertion_start, move_size);
    }
    ++arr->values_used;
    return insertion_start;
}
void *cecs_array_insert_many(cecs_array *arr, const size_t index, const size_t count, const size_t value_size) {
    if (cecs_expect_not(index > arr->values_used)) {
        cecs_debugbreak_fail_message("error: attempted to insert in cecs_array with end index greater than count");
    } else if (cecs_expect_not(arr->values_used + count > arr->values_capacity)) {
        cecs_debugbreak_fail_message("error: attempted to insert in cecs_array exceeding its capacity");
    }

    unsigned char *const insertion_start = arr->values + index * value_size;
    unsigned char *const insertion_end = insertion_start + count * value_size;
    const size_t move_count = arr->values_used - index;
    const size_t move_size = move_count * value_size;
    if (move_count <= count) {
        memcpy(insertion_end, insertion_start, move_size);
    } else {
        memmove(insertion_end, insertion_start, move_size);
    }
    arr->values_used += count;
    return insertion_start;
}
void *cecs_array_insert_many_copy(cecs_array *arr, const size_t index, const void *values, const size_t count, const size_t value_size) {
    void *const elements = cecs_array_insert_many(arr, index, count, value_size);
    return memcpy(elements, values, count * value_size);
}

void cecs_array_pop(cecs_array *arr) {
    cecs_debugbreak_fail_unless(
        arr->values_used > 0,
        "error: attempted to pop from empty cecs_array"
    );
    --arr->values_used;
}
void cecs_array_truncate(cecs_array *arr, const size_t new_count) {
    cecs_debugbreak_fail_unless(
        new_count <= arr->values_used,
        "error: attempted to truncate cecs_array to a larger count"
    );
    arr->values_used = new_count;
}
void cecs_array_swap_last_pop(cecs_array *arr, const size_t index, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        arr->values_used > 0,
        "error: attempted to swap-last-pop from empty cecs_array"
    );
    switch (arr->values_used) {
    case 0: {
        cecs_unreachable();
        break;
    }
    case 1: {
        arr->values_used = 0;
        break;
    }
    default: {
        void *const swapped = cecs_array_get_mut(arr, index, value_size);
        const void *const last = cecs_array_last(arr, value_size);
        memcpy(swapped, last, value_size);
        --arr->values_used;
        break;
    }
    }
}

void cecs_array_remove(cecs_array *arr, const size_t index, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        index < arr->values_used,
        "error: attempted to remove from cecs_array with out-of-bounds index"
    );
    if (index + 1 < arr->values_used) {
        memmove(
            arr->values + index * value_size,
            arr->values + (index + 1) * value_size,
            (arr->values_used - index - 1) * value_size
        );
    }
    --arr->values_used;
}
void cecs_array_remove_many(cecs_array *arr, const size_t index, const size_t count, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        index + count <= arr->values_used,
        "error: attempted to remove from cecs_array with out-of-bounds index"
    );
    if (index + count < arr->values_used) {
        memmove(
            arr->values + index * value_size,
            arr->values + (index + count) * value_size,
            (arr->values_used - index - count) * value_size
        );
    }
    arr->values_used -= count;
}

extern inline void *cecs_array_get_ptr(const cecs_array *arr, const size_t index, const size_t size) {
    cecs_debugbreak_fail_unless(
        index < arr->values_used,
        "error: attempted to get element from cecs_array with out-of-bounds index"
    );
    return arr->values + index * size;
}
const void *cecs_array_get(const cecs_array *arr, const size_t index, const size_t size) {
    return cecs_array_get_ptr(arr, index, size);
}
void *cecs_array_get_mut(cecs_array *arr, const size_t index, const size_t size) {
    return cecs_array_get_ptr(arr, index, size);
}
extern inline void *cecs_array_get_range_ptr(const cecs_array *arr, const size_t index, const size_t count, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        index + count <= arr->values_used,
        "error: attempted to get range from cecs_array with out-of-bounds range"
    );
    return arr->values + index * value_size;
}
const void *cecs_array_get_range(const cecs_array *arr, const size_t index, const size_t count, const size_t value_size) {
    return cecs_array_get_range_ptr(arr, index, count, value_size);
}
void *cecs_array_get_range_mut(cecs_array *arr, const size_t index, const size_t count, const size_t value_size) {
    return cecs_array_get_range_ptr(arr, index, count, value_size);
}


// static inline const unsigned char *cecs_dynarray_values(const cecs_dynarray *arr) {
//     return arr->array.values;
// }
// static inline unsigned char *cecs_dynarray_values_mut(cecs_dynarray *arr) {
//     return arr->array.values;
// }
cecs_dynarray cecs_dynarray_create_with_capacity(cecs_allocator *a, const size_t values_capacity, const size_t value_size) {
    if (value_size == 0 || values_capacity == 0) {
        return cecs_dynarray_create();
    } else {
        void *const values = cecs_allocator_alloc_aligned(a,  value_size * values_capacity, cecs_max_alignment_from_size(value_size));
        return (cecs_dynarray) {
            .array = cecs_array_create(values, values_capacity)
        };
    }
}
void cecs_dynarray_destroy(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size) {
    if (arr->array.values) {
        cecs_allocator_free(a, arr->array.values, value_size * arr->array.values_capacity);
        arr->array.values = NULL;
    }
    arr->array.values_used = 0;
    arr->array.values_capacity = 0;
}

void cecs_dynarray_reserve_exact(cecs_dynarray* arr, cecs_allocator* a, const size_t additional_capacity, const size_t value_size) {
    const size_t current_capacity = cecs_dynarray_capacity(arr);
    const size_t requested_capacity = cecs_dynarray_count(arr) + additional_capacity;
    if (current_capacity < requested_capacity) {
        arr->array.values = cecs_allocator_realloc_aligned(
            a,
            arr->array.values,
            value_size * current_capacity,
            value_size * requested_capacity,
            cecs_max_alignment_from_size(value_size) // TODO: ask for aignment
        );
        arr->array.values_capacity = requested_capacity;
    }
}
void cecs_dynarray_reserve(cecs_dynarray* arr, cecs_allocator* a, const size_t additional_capacity, const size_t value_size) {
    cecs_dynarray_reserve_exact(arr, a, cecs_max(additional_capacity, cecs_dynarray_count(arr) << 1ull), value_size);
}

void cecs_dynarray_shrink_exact(cecs_dynarray *arr, cecs_allocator *a, const size_t values_new_capacity, const size_t value_size) {
    if (cecs_expect_not(values_new_capacity < arr->array.values_used)) {
        cecs_debugbreak_fail_message(
            "fatal error: attempted to shrink dynamic array to smaller capacity than used."
            "Use cecs_dynarray_truncate() to truncate the array before shrinking."
        );
    } else if (values_new_capacity < arr->array.values_capacity) {
        arr->array.values = cecs_allocator_realloc_aligned(
            a,
            arr->array.values,
            value_size * arr->array.values_capacity,
            value_size * values_new_capacity,
            cecs_max_alignment_from_size(value_size)
        );
        arr->array.values_capacity = values_new_capacity;
    } else if (cecs_expect_not(values_new_capacity > arr->array.values_capacity)) {
        cecs_debugbreak_fail_message("fatal error: attempted to shrink dynamic array to a capacity larger or equal than current capacity");
    }
}
void cecs_dynarray_shrink(cecs_dynarray *arr, cecs_allocator *a, const size_t values_new_capacity, const size_t value_size) {
    cecs_dynarray_shrink_exact(arr, a, cecs_max(values_new_capacity, arr->array.values_used), value_size);
}
void cecs_dynarray_shrink_to_fit(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size) {
    cecs_dynarray_shrink_exact(arr, a, arr->array.values_used, value_size);
}

void *cecs_dynarray_push(cecs_dynarray *arr, cecs_allocator *a, const size_t value_size) {
    cecs_dynarray_reserve(arr, a, 1ull, value_size);
    return cecs_array_push(&arr->array, value_size);
}
void *cecs_dynarray_push_many(cecs_dynarray *arr, cecs_allocator *a, const size_t count, const size_t value_size) {
    cecs_dynarray_reserve(arr, a, count, value_size);
    return cecs_array_push_many(&arr->array, count, value_size);
}
void *cecs_dynarray_push_many_copy(cecs_dynarray *arr, cecs_allocator *a, const void *values, const size_t count, const size_t value_size) {
    void *const elements = cecs_dynarray_push_many(arr, a, count, value_size);
    return memcpy(elements, values, count * value_size);
}

void *cecs_dynarray_extend(cecs_dynarray *arr, cecs_allocator *a, const size_t start_index_inclusive, const size_t end_index_exclusive, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        start_index_inclusive <= end_index_exclusive,
        "error: attempted to extend cecs_dynarray with start index greater than end index"
    );
    cecs_dynarray_reserve(arr, a, end_index_exclusive - start_index_inclusive, value_size);
    return cecs_array_extend(&arr->array, start_index_inclusive, end_index_exclusive, value_size);
}
void *cecs_dynarray_insert(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t value_size) {
    cecs_dynarray_reserve(arr, a, 1ull, value_size);
    return cecs_array_insert(&arr->array, index, value_size);
}
void *cecs_dynarray_insert_many(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const size_t count, const size_t value_size) {
    cecs_dynarray_reserve(arr, a, count, value_size);
    return cecs_array_insert_many(&arr->array, index, count, value_size);
}
void *cecs_dynarray_insert_many_copy(cecs_dynarray *arr, cecs_allocator *a, const size_t index, const void *values, const size_t count, const size_t value_size) {
    void *const elements = cecs_dynarray_insert_many(arr, a, index, count, value_size);
    return memcpy(elements, values, count * value_size);
}

void cecs_dynarray_pop(cecs_dynarray *arr) {
    cecs_array_pop(&arr->array);
}
void cecs_dynarray_truncate(cecs_dynarray *arr, const size_t new_count) {
    cecs_array_truncate(&arr->array, new_count);
}
void cecs_dynarray_swap_last_pop(cecs_dynarray* arr, const size_t index, const size_t value_size) {
    cecs_array_swap_last_pop(&arr->array, index, value_size);
}

void cecs_dynarray_remove(cecs_dynarray* arr, const size_t index, const size_t value_size) {
    cecs_array_remove(&arr->array, index, value_size);
}
void cecs_dynarray_remove_many(cecs_dynarray* arr, const size_t index, const size_t count, const size_t value_size) {
    cecs_array_remove_many(&arr->array, index, count, value_size);
}

extern inline void *cecs_dynarray_get_ptr(const cecs_dynarray *arr, const size_t index, const size_t size) {
    return cecs_array_get_ptr(&arr->array, index, size);
}
void* cecs_dynarray_get_mut(cecs_dynarray* arr, const size_t index, const size_t size) {
    return cecs_dynarray_get_ptr(arr, index, size);
}
const void *cecs_dynarray_get(const cecs_dynarray *arr, const size_t index, const size_t size) {
    return cecs_dynarray_get_ptr(arr, index, size);
}
extern inline void *cecs_dynarray_get_range_ptr(const cecs_dynarray *arr, const size_t index, const size_t count, const size_t size) {
    return cecs_array_get_range_ptr(&arr->array, index, count, size);
}
void *cecs_dynarray_get_range_mut(cecs_dynarray* arr, const size_t index, const size_t count, const size_t value_size) {
    return cecs_dynarray_get_range_ptr(arr, index, count, value_size);
}
const void *cecs_dynarray_get_range(const cecs_dynarray *arr, const size_t index, const size_t count, const size_t value_size) {
    return cecs_dynarray_get_range_ptr(arr, index, count, value_size);
}
