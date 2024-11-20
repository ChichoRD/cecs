#include "cecs_queue.h"

cecs_queue cecs_queue_create(void) {
    return (cecs_queue) {
        .elements = cecs_dynamic_array_create(),
            .first = 0
    };
}

cecs_queue cecs_queue_create_with_capacity(cecs_arena* a, size_t capacity) {
    return (cecs_queue) {
        .elements = cecs_dynamic_array_create_with_capacity(a, capacity),
            .first = 0
    };
}

void* cecs_queue_get(cecs_queue* q, size_t index, size_t size) {
    assert(index < cecs_queue_count_of_size(q, size) && "Attempted to get element with index out of bounds");
    return cecs_dynamic_array_get(&q->elements, q->first + index, size);
}

size_t cecs_queue_pop_first(cecs_queue* q, cecs_arena* a, void* out_pop_element, size_t size) {
    size_t cecs_dynamic_array_count = cecs_dynamic_array_count_of_size(&q->elements, size);
    assert(q->first < cecs_dynamic_array_count && "Attempted to pop first element of empty queue");
    memcpy(out_pop_element, cecs_dynamic_array_get(&q->elements, q->first++, size), size);

    size_t remaining_count = cecs_queue_count_of_size(q, size);
    if (q->first == cecs_dynamic_array_count) {
        cecs_dynamic_array_clear(&q->elements);
        q->first = 0;
    }
    else if (q->first >= (cecs_dynamic_array_count + 1) / 2) {
        cecs_dynamic_array_set_range(
            &q->elements,
            0,
            cecs_dynamic_array_get_range(&q->elements, q->first, remaining_count, size),
            remaining_count,
            size
        );
        cecs_dynamic_array_remove_range(&q->elements, a, remaining_count, cecs_dynamic_array_count - remaining_count, size);
        q->first = 0;
    }
    return remaining_count;
}

size_t cecs_queue_pop_last(cecs_queue* q, cecs_arena* a, void* out_pop_element, size_t size) {
    size_t cecs_dynamic_array_count = cecs_dynamic_array_count_of_size(&q->elements, size);
    assert(cecs_dynamic_array_count > 0 && "Attempted to pop last element of empty queue");
    memcpy(out_pop_element, cecs_dynamic_array_last(&q->elements, size), size);
    cecs_dynamic_array_remove(&q->elements, a, cecs_dynamic_array_count - 1, size);

    return cecs_queue_count_of_size(q, size);
}

void* cecs_queue_push_first(cecs_queue* q, cecs_arena* a, void* element, size_t size) {
    if (q->first == 0) {
        return cecs_dynamic_array_insert(&q->elements, a, 0, element, size);
    }
    else {
        return cecs_dynamic_array_set(&q->elements, --q->first, element, size);
    }
}

void* cecs_queue_push_last(cecs_queue* q, cecs_arena* a, void* element, size_t size) {
    return cecs_dynamic_array_add(&q->elements, a, element, size);
}
