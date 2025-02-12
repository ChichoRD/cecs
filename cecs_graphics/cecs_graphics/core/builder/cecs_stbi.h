#ifndef CECS_STBI_H
#define CECS_STBI_H

#include <stb_image.h>
#include <cecs_core/containers/cecs_arena.h>

typedef struct cecs_stbi_allocator {
    cecs_arena *current_arena;
} cecs_stbi_allocator;
cecs_stbi_allocator *cecs_stbi_allocator_get_current_allocator(void);

#endif