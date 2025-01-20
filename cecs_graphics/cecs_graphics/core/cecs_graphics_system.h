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


typedef cecs_raw_stream cecs_uniform_raw_stream;
CECS_COMPONENT_DECLARE(cecs_uniform_raw_stream);

const cecs_uniform_raw_stream *cecs_graphics_system_set_component_as_uniform(
    cecs_graphics_system *system,
    cecs_world *world,
    cecs_entity_id entity,
    cecs_component_id component_id,
    void *component,
    size_t size
);
#define CECS_GRAPHICS_SYSTEM_SET_COMPONENT_AS_UNIFORM(type, graphics_system_ref, world_ref, entity, component_ref) \
    (cecs_graphics_system_set_component_as_uniform(graphics_system_ref, world_ref, entity, CECS_COMPONENT_ID(type), component_ref, sizeof(type)))

cecs_buffer_storage_attachment *cecs_graphics_system_sync_uniform_components(
    cecs_graphics_system *system,
    cecs_world *world,
    cecs_component_id component_id
);
#define CECS_GRAPHICS_SYSTEM_SYNC_UNIFORM_COMPONENTS(type, grpahics_system_ref, world_ref) \
    (cecs_graphics_system_sync_uniform_components(grpahics_system_ref, world_ref, CECS_COMPONENT_ID(type)))


#endif