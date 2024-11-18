#include "queue.h"

queue queue_create() {
    return (queue) {
        .elements = list_create(),
            .first = 0
    };
}

queue queue_create_with_capacity(arena* a, size_t capacity) {
    return (queue) {
        .elements = list_create_with_capacity(a, capacity),
            .first = 0
    };
}

void* queue_get(queue* q, size_t index, size_t size) {
    assert(index < queue_count_of_size(q, size) && "Attempted to get element with index out of bounds");
    return list_get(&q->elements, q->first + index, size);
}

size_t queue_pop_first(queue* q, arena* a, void* out_pop_element, size_t size) {
    size_t list_count = list_count_of_size(&q->elements, size);
    assert(q->first < list_count && "Attempted to pop first element of empty queue");
    memcpy(out_pop_element, list_get(&q->elements, q->first++, size), size);

    size_t remaining_count = queue_count_of_size(q, size);
    if (q->first == list_count) {
        list_clear(&q->elements);
        q->first = 0;
    }
    else if (q->first >= (list_count + 1) / 2) {
        list_set_range(
            &q->elements,
            0,
            list_get_range(&q->elements, q->first, remaining_count, size),
            remaining_count,
            size
        );
        list_remove_range(&q->elements, a, remaining_count, list_count - remaining_count, size);
        q->first = 0;
    }
    return remaining_count;
}

size_t queue_pop_last(queue* q, arena* a, void* out_pop_element, size_t size) {
    size_t list_count = list_count_of_size(&q->elements, size);
    assert(list_count > 0 && "Attempted to pop last element of empty queue");
    memcpy(out_pop_element, list_last(&q->elements, size), size);
    list_remove(&q->elements, a, list_count - 1, size);

    return queue_count_of_size(q, size);
}

void* queue_push_first(queue* q, arena* a, void* element, size_t size) {
    if (q->first == 0) {
        return list_insert(&q->elements, a, 0, element, size);
    }
    else {
        return list_set(&q->elements, --q->first, element, size);
    }
}

void* queue_push_last(queue* q, arena* a, void* element, size_t size) {
    return list_add(&q->elements, a, element, size);
}
