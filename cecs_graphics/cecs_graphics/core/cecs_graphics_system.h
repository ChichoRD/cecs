#ifndef CECSC_GRAPHICS_SYSTEM_H
#define CECSC_GRAPHICS_SYSTEM_H

#include "cecs_graphics_world.h"
#include "context/cecs_graphics_context.h"

typedef struct cecs_graphics_system {
    cecs_graphics_world world; 
    cecs_graphics_context context;
} cecs_graphics_system;

cecs_graphics_system cecs_graphics_system_create(size_t vertex_capacity, size_t vertex_attributes_capacity, GLFWwindow *window);
cecs_graphics_system cecs_graphics_system_create_offscreen(size_t vertex_capacity, size_t vertex_attributes_capacity);
void cecs_graphics_system_free(cecs_graphics_system *s);

#endif