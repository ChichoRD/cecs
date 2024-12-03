#ifndef CECS_GRAPHICS_WORLD_H
#define CECS_GRAPHICS_WORLD_H

#include <cecs/cecs.h>

typedef	struct cecs_graphics_world {
    cecs_world world;
} cecs_graphics_world;

cecs_graphics_world cecs_graphics_world_create(size_t vertex_capacity, size_t vertex_attributes_capacity);
void cecs_graphics_world_free(cecs_graphics_world *w);

#endif