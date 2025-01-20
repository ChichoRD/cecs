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

cecs_buffer_storage_attachment *cecs_graphics_world_get_buffer_attachments(cecs_graphics_world *graphics_world, cecs_component_id component_id);
#define CECS_GRAPHICS_WORLD_GET_BUFFER_ATTACHMENTS(type, graphics_world_ref) \
    cecs_graphics_world_get_buffer_attachments(graphics_world_ref, CECS_COMPONENT_ID(type))

cecs_buffer_storage_attachment *cecs_graphics_world_get_or_set_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_component_id component_id,
    cecs_buffer_storage_attachment *default_attachments
);
#define CECS_GRAPHICS_WORLD_GET_OR_SET_BUFFER_ATTACHMENTS(type, graphics_world_ref, default_attachments_ref) \
    cecs_graphics_world_get_or_set_buffer_attachments(graphics_world_ref, CECS_COMPONENT_ID(type), default_attachments_ref)

cecs_exclusive_index_buffer_pair cecs_graphics_world_get_index_buffers(cecs_graphics_world *graphics_world);

cecs_arena *cecs_graphics_world_default_buffer_arena(cecs_graphics_world *graphics_world);
cecs_buffer_storage_attachment *cecs_graphics_world_get_or_init_uniform_buffer(
    cecs_graphics_world *world,
    cecs_graphics_context *context,
    cecs_component_id component_id,
    size_t size
);

cecs_entity_id cecs_world_add_entity_with_mesh(
    cecs_world *world,
    cecs_mesh *mesh
);
cecs_entity_id cecs_world_add_entity_with_indexed_mesh(
    cecs_world *world,
    cecs_mesh *mesh,
    cecs_index_stream *index_stream
);

#endif