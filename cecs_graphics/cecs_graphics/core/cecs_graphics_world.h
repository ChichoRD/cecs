#ifndef CECS_GRAPHICS_WORLD_H
#define CECS_GRAPHICS_WORLD_H

#include <cecs_core/cecs_core.h>
#include "component/cecs_vertex.h"

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

#endif