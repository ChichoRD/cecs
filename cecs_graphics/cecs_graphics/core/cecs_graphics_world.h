#ifndef CECS_GRAPHICS_WORLD_H
#define CECS_GRAPHICS_WORLD_H

#include <cecs_core/cecs_core.h>
#include "component/cecs_vertex.h"
#include "component/cecs_mesh.h"
#include "context/cecs_graphics_context.h"

typedef	struct cecs_graphics_world {
    cecs_world world;
} cecs_graphics_world;

cecs_graphics_world cecs_graphics_world_create(size_t vertex_capacity, size_t vertex_attributes_capacity);
void cecs_graphics_world_free(cecs_graphics_world *w);

#define CECS_GRAPHICS_WORLD_GET_BUFFER_ATTACHMENTS(type, graphics_world_ref) \
    CECS_WORLD_GET_COMPONENT_STORAGE_ATTACHMENTS(type, cecs_buffer_storage_attachment, (&(graphics_world_ref)->world))

#define CECS_GRAPHICS_WORLD_GET_OR_SET_BUFFER_ATTACHMENTS(type, graphics_world_ref, default_attachments_ref) \
    CECS_WORLD_GET_OR_SET_COMPONENT_STORAGE_ATTACHMENTS(type, cecs_buffer_storage_attachment, (&(graphics_world_ref)->world), default_attachments_ref)

cecs_exclusive_index_buffer_pair cecs_graphics_world_get_index_buffers(cecs_graphics_world *graphics_world);

cecs_arena *cecs_graphics_world_default_buffer_arena(cecs_graphics_world *graphics_world);

cecs_entity_id cecs_world_add_entity_with_mesh(
    cecs_world *world,
    cecs_mesh *mesh
);
cecs_entity_id cecs_world_add_entity_with_indexed_mesh(
    cecs_world *world,
    cecs_mesh *mesh,
    cecs_index_stream *index_stream
);

typedef cecs_raw_stream cecs_uniform_raw_stream;
CECS_COMPONENT_DECLARE(cecs_uniform_raw_stream);

const cecs_uniform_raw_stream *cecs_world_set_component_as_uniform(
    cecs_world *world,
    cecs_graphics_context *context,
    cecs_entity_id entity,
    cecs_component_id component_id,
    void *component,
    size_t size
);
#define CECS_WORLD_SET_COMPONENT_AS_UNIFORM(type, world_ref, context_ref, entity, component_ref) \
    (cecs_world_set_component_as_uniform(world_ref, context_ref, entity, CECS_COMPONENT_ID(type), component_ref, sizeof(type)))

#endif