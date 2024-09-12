#ifndef QUEUE_H
#define QUEUE_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "list.h"

typedef struct queue {
    list elements;
    size_t first;
} queue;

queue queue_create() {
    return (queue) {
        .elements = list_create(),
        .first = 0
    };
}

queue queue_create_with_capacity(arena *a, size_t capacity) {
    return (queue) {
        .elements = list_create_with_capacity(a, capacity),
        .first = 0
    };
}

inline size_t queue_count_of_size(const queue *q, size_t size) {
    return list_count_of_size(&q->elements, size) - q->first;
}
#define QUEUE_COUNT(type, queue_ref) queue_count_of_size(queue_ref, sizeof(type))

void *queue_get(queue *q, size_t index, size_t size) {
    assert(index < queue_count_of_size(q, size) && "Attempted to get element with index out of bounds");
    return list_get(&q->elements, q->first + index, size);
}
#define QUEUE_GET(type, queue_ref, index) ((type *)queue_get(queue_ref, index, sizeof(type)))

inline void *queue_first(const queue *q, size_t size) {
    assert(q->first < list_count_of_size(&q->elements, size) && "Attempted to get first element of empty queue");
    return list_get(&q->elements, q->first, size);
}
#define QUEUE_FIRST(type, queue_ref) ((type *)queue_first(queue_ref, sizeof(type)))

inline void *queue_last(const queue *q, size_t size) {
    assert(list_count_of_size(&q->elements, size) > 0 && "Attempted to get last element of empty queue");
    return list_last(&q->elements, size);
}
#define QUEUE_LAST(type, queue_ref) ((type *)queue_last(queue_ref, sizeof(type)))

size_t queue_pop_first(queue *q, arena *a, void *out_pop_element, size_t size) {
    size_t list_count = list_count_of_size(&q->elements, size);
    assert(q->first < list_count && "Attempted to pop first element of empty queue");
    memcpy(out_pop_element, list_get(&q->elements, q->first++, size), size);
    
    size_t remaining_count = queue_count_of_size(q, size);
    if (q->first == list_count) {
        list_clear(&q->elements);
        q->first = 0;
    } else if (q->first >= (list_count + 1) / 2) {
        list_set_range(
            &q->elements,
            0,
            list_get_range(&q->elements, q->first, remaining_count, size),
            remaining_count,
            size
        );
        list_remove_range(&q->elements, a, q->first, remaining_count, size);
        q->first = 0;
    }
    
    return remaining_count;
}
#define QUEUE_POP_FIRST(type, queue_ref, arena_ref, out_pop_element) \
    queue_pop_first(queue_ref, arena_ref, out_pop_element, sizeof(type))

size_t queue_pop_last(queue *q, arena *a, void *out_pop_element, size_t size) {
    size_t list_count = list_count_of_size(&q->elements, size);
    assert(list_count > 0 && "Attempted to pop last element of empty queue");
    memcpy(out_pop_element, list_last(&q->elements, size), size);
    list_remove(&q->elements, a, list_count - 1, size);

    return queue_count_of_size(q, size);
}
#define QUEUE_POP_LAST(type, queue_ref, arena_ref, out_pop_element) \
    queue_pop_last(queue_ref, arena_ref, out_pop_element, sizeof(type))

void *queue_push_first(queue *q, arena *a, void *element, size_t size) {
    if (q->first == 0) {
        return list_insert(&q->elements, a, 0, element, size);
    } else {
        return list_set(&q->elements, --q->first, element, size);
    }
}
#define QUEUE_PUSH_FIRST(type, queue_ref, arena_ref, element_ref) \
    ((type *)queue_push_first(queue_ref, arena_ref, element_ref, sizeof(type)))

void *queue_push_last(queue *q, arena *a, void *element, size_t size) {
    return list_add(&q->elements, a, element, size);
}
#define QUEUE_PUSH_LAST(type, queue_ref, arena_ref, element_ref) \
    ((type *)queue_push_last(queue_ref, arena_ref, element_ref, sizeof(type)))


#endif