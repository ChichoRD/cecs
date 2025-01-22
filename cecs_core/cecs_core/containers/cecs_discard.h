#ifndef CECS_DISCARD_H
#define CECS_DISCARD_H

#include <stdint.h>
#include "cecs_arena.h"

typedef struct cecs_discard {
    void *handle;
    size_t size;
} cecs_discard;

static inline cecs_discard cecs_discard_create(void) {
    return (cecs_discard){ .handle = NULL, .size = 0 };
}

cecs_discard cecs_discard_create_with_size(cecs_arena *a, size_t size);

size_t cecs_discard_ensure(cecs_discard *discard, cecs_arena *a, size_t size);
void *cecs_discard_use(cecs_discard *discard, cecs_arena *a, size_t size);

#endif