#ifndef CECS_BUMP_ALLOCATOR_H
#define CECS_BUMP_ALLOCATOR_H

#include "cecs_allocation.h"

typedef struct cecs_bump_allocator {
    void *next;
    void *const block_start;
    void *const block_end;
} cecs_bump_allocator;

#endif