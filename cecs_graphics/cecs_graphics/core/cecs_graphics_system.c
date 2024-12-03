#include "cecs_graphics_system.h"

cecs_graphics_system cecs_graphics_system_create(size_t vertex_capacity, size_t vertex_attributes_capacity, GLFWwindow *window) {
    return (cecs_graphics_system){
        .world = cecs_graphics_world_create(vertex_capacity, vertex_attributes_capacity),
        .context = cecs_graphics_context_create(window)
    };
}

cecs_graphics_system cecs_graphics_system_create_offscreen(size_t vertex_capacity, size_t vertex_attributes_capacity) {
    return (cecs_graphics_system){
        .world = cecs_graphics_world_create(vertex_capacity, vertex_attributes_capacity),
        .context = cecs_graphics_context_create_offscreen()
    };
}

void cecs_graphics_system_free(cecs_graphics_system *s) {
    cecs_graphics_world_free(&s->world);
}