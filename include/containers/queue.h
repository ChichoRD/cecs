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

queue queue_create();

queue queue_create_with_capacity(arena *a, size_t capacity);

inline size_t queue_count_of_size(const queue *q, size_t size) {
    return list_count_of_size(&q->elements, size) - q->first;
}
#define QUEUE_COUNT(type, queue_ref) queue_count_of_size(queue_ref, sizeof(type))

void *queue_get(queue *q, size_t index, size_t size);
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

size_t queue_pop_first(queue *q, arena *a, void *out_pop_element, size_t size);
#define QUEUE_POP_FIRST(type, queue_ref, arena_ref, out_pop_element) \
    queue_pop_first(queue_ref, arena_ref, out_pop_element, sizeof(type))

size_t queue_pop_last(queue *q, arena *a, void *out_pop_element, size_t size);
#define QUEUE_POP_LAST(type, queue_ref, arena_ref, out_pop_element) \
    queue_pop_last(queue_ref, arena_ref, out_pop_element, sizeof(type))

void *queue_push_first(queue *q, arena *a, void *element, size_t size);
#define QUEUE_PUSH_FIRST(type, queue_ref, arena_ref, element_ref) \
    ((type *)queue_push_first(queue_ref, arena_ref, element_ref, sizeof(type)))

void *queue_push_last(queue *q, arena *a, void *element, size_t size);
#define QUEUE_PUSH_LAST(type, queue_ref, arena_ref, element_ref) \
    ((type *)queue_push_last(queue_ref, arena_ref, element_ref, sizeof(type)))


#endif