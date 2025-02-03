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

static inline cecs_vertex_storage_attachment *cecs_graphics_world_get_vertex_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_vertex_attribute_id attribute_id
) {
    cecs_buffer_storage_attachment *buffer = cecs_graphics_world_get_buffer_attachments(graphics_world, attribute_id);
    CECS_UNION_IS_ASSERT(cecs_vertex_storage_attachment, cecs_stream_storage_attachment, buffer->stream);
    return &CECS_UNION_GET_UNCHECKED(
        cecs_vertex_storage_attachment,
        buffer->stream
    );
}

static inline cecs_index_storage_attachment *cecs_graphics_world_get_index_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_vertex_index_id index_id
) {
    cecs_buffer_storage_attachment *buffer = cecs_graphics_world_get_buffer_attachments(graphics_world, index_id);
    CECS_UNION_IS_ASSERT(cecs_index_storage_attachment, cecs_stream_storage_attachment, buffer->stream);
    return &CECS_UNION_GET_UNCHECKED(
        cecs_index_storage_attachment,
        buffer->stream
    );
}

static inline cecs_uniform_storage_attachment *cecs_graphics_world_get_uniform_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_component_id component_id
) {
    cecs_buffer_storage_attachment *buffer = cecs_graphics_world_get_buffer_attachments(graphics_world, component_id);
    CECS_UNION_IS_ASSERT(cecs_uniform_storage_attachment, cecs_stream_storage_attachment, buffer->stream);
    return &CECS_UNION_GET_UNCHECKED(
        cecs_uniform_storage_attachment,
        buffer->stream
    );
}

cecs_buffer_storage_attachment *cecs_graphics_world_get_or_set_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_component_id component_id,
    cecs_buffer_storage_attachment *default_attachments
);
#define CECS_GRAPHICS_WORLD_GET_OR_SET_BUFFER_ATTACHMENTS(type, graphics_world_ref, default_attachments_ref) \
    cecs_graphics_world_get_or_set_buffer_attachments(graphics_world_ref, CECS_COMPONENT_ID(type), default_attachments_ref)

static inline cecs_vertex_storage_attachment *cecs_graphics_world_get_or_set_vertex_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_vertex_attribute_id attribute_id,
    cecs_buffer_storage_attachment *default_attachment
) {
    cecs_buffer_storage_attachment *buffer = cecs_graphics_world_get_or_set_buffer_attachments(graphics_world, attribute_id, default_attachment);
    CECS_UNION_IS_ASSERT(cecs_vertex_storage_attachment, cecs_stream_storage_attachment, buffer->stream);
    return &CECS_UNION_GET_UNCHECKED(
        cecs_vertex_storage_attachment,
        buffer->stream
    );
}

static inline cecs_instance_storage_attachment *cecs_graphics_world_get_or_set_instance_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_instance_attribute_id attribute_id,
    cecs_buffer_storage_attachment *default_attachment
) {
    cecs_buffer_storage_attachment *buffer = cecs_graphics_world_get_or_set_buffer_attachments(graphics_world, attribute_id, default_attachment);
    CECS_UNION_IS_ASSERT(cecs_instance_storage_attachment, cecs_stream_storage_attachment, buffer->stream);
    return &CECS_UNION_GET_UNCHECKED(
        cecs_instance_storage_attachment,
        buffer->stream
    );
}

static inline cecs_index_storage_attachment *cecs_graphics_world_get_or_set_index_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_vertex_index_id index_id,
    cecs_buffer_storage_attachment *default_attachment
) {
    cecs_buffer_storage_attachment *buffer = cecs_graphics_world_get_or_set_buffer_attachments(graphics_world, index_id, default_attachment);
    CECS_UNION_IS_ASSERT(cecs_index_storage_attachment, cecs_stream_storage_attachment, buffer->stream);
    return &CECS_UNION_GET_UNCHECKED(
        cecs_index_storage_attachment,
        buffer->stream
    );
}

static inline cecs_uniform_storage_attachment *cecs_graphics_world_get_or_set_uniform_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_component_id component_id,
    cecs_buffer_storage_attachment *default_attachment
) {
    cecs_buffer_storage_attachment *buffer = cecs_graphics_world_get_or_set_buffer_attachments(graphics_world, component_id, default_attachment);
    CECS_UNION_IS_ASSERT(cecs_uniform_storage_attachment, cecs_stream_storage_attachment, buffer->stream);
    return &CECS_UNION_GET_UNCHECKED(
        cecs_uniform_storage_attachment,
        buffer->stream
    );
}

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