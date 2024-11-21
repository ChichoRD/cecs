#ifndef CECS_QUEUE_H
#define CECS_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "cecs_dynamic_array.h"

typedef struct cecs_queue {
    cecs_dynamic_array elements;
    size_t first;
} cecs_queue;

cecs_queue cecs_queue_create(void);

cecs_queue cecs_queue_create_with_capacity(cecs_arena *a, size_t capacity);

inline size_t cecs_queue_count_of_size(const cecs_queue *q, size_t size) {
    return cecs_dynamic_array_count_of_size(&q->elements, size) - q->first;
}
#define CECS_QUEUE_COUNT(type, queue_ref) cecs_queue_count_of_size(queue_ref, sizeof(type))

void *cecs_queue_get(cecs_queue *q, size_t index, size_t size);
#define CECS_QUEUE_GET(type, queue_ref, index) ((type *)cecs_queue_get(queue_ref, index, sizeof(type)))

inline void *cecs_queue_first(const cecs_queue *q, size_t size) {
    assert(q->first < cecs_dynamic_array_count_of_size(&q->elements, size) && "Attempted to get first element of empty queue");
    return cecs_dynamic_array_get(&q->elements, q->first, size);
}
#define CECS_QUEUE_FIRST(type, queue_ref) ((type *)cecs_queue_first(queue_ref, sizeof(type)))

inline void *cecs_queue_last(const cecs_queue *q, size_t size) {
    assert(cecs_dynamic_array_count_of_size(&q->elements, size) > 0 && "Attempted to get last element of empty queue");
    return cecs_dynamic_array_last(&q->elements, size);
}
#define CECS_QUEUE_LAST(type, queue_ref) ((type *)cecs_queue_last(queue_ref, sizeof(type)))

size_t cecs_queue_pop_first(cecs_queue *q, cecs_arena *a, void *out_pop_element, size_t size);
#define CECS_QUEUE_POP_FIRST(type, queue_ref, arena_ref, out_pop_element) \
    cecs_queue_pop_first(queue_ref, arena_ref, out_pop_element, sizeof(type))

size_t cecs_queue_pop_last(cecs_queue *q, cecs_arena *a, void *out_pop_element, size_t size);
#define CECS_QUEUE_POP_LAST(type, queue_ref, arena_ref, out_pop_element) \
    cecs_queue_pop_last(queue_ref, arena_ref, out_pop_element, sizeof(type))

void *cecs_queue_push_first(cecs_queue *q, cecs_arena *a, void *element, size_t size);
#define CECS_QUEUE_PUSH_FIRST(type, queue_ref, arena_ref, element_ref) \
    ((type *)queue_push_first(queue_ref, arena_ref, element_ref, sizeof(type)))

void *cecs_queue_push_last(cecs_queue *q, cecs_arena *a, void *element, size_t size);
#define CECS_QUEUE_PUSH_LAST(type, queue_ref, arena_ref, element_ref) \
    ((type *)cecs_queue_push_last(queue_ref, arena_ref, element_ref, sizeof(type)))


#endif