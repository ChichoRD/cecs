#include "cecs_graphics_world.h"

cecs_graphics_world cecs_graphics_world_create(size_t vertex_capacity, size_t vertex_attributes_capacity) {
    return (cecs_graphics_world){
        .world = cecs_world_create(
            vertex_capacity,
            vertex_attributes_capacity,
            1
        )
    };
}

void cecs_graphics_world_free(cecs_graphics_world *w) {
    cecs_world_free(&w->world);
}