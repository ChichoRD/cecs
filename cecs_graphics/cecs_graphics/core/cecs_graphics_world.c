#include "cecs_graphics_world.h"

cecs_graphics_world cecs_graphics_world_create(size_t vertex_capacity, size_t vertex_attributes_capacity) {
    return (cecs_graphics_world){
        .world = cecs_world_create(
            vertex_capacity,
            vertex_attributes_capacity,
            vertex_attributes_capacity
        )
    };
}

void cecs_graphics_world_free(cecs_graphics_world *w) { 
    for (
        cecs_world_components_iterator it = cecs_world_components_iterator_create(&w->world.components);
        !cecs_world_components_iterator_done(&it);
        cecs_world_components_iterator_next(&it)
    ) {
        cecs_associated_component_storage storage = cecs_world_components_iterator_current(&it);
        if (cecs_world_has_component_storage_attachments(&w->world, storage.component_id)) {
            const cecs_component_storage_attachments *attachments = cecs_world_components_get_component_storage_attachments_unchecked(
                &w->world.components,
                storage.component_id
            );
            
            if (attachments->flags & cecs_component_storage_attachment_usage_graphics_buffer) {
                cecs_buffer_storage_attachment *buffer_attachment = attachments->user_attachments;
                if (buffer_attachment->buffer_flags & cecs_buffer_flags_initialized) {
                    cecs_buffer_storage_attachment_free(buffer_attachment);
                }
            }
        }
    }
    cecs_world_free(&w->world);
}

cecs_buffer_storage_attachment *cecs_graphics_world_get_buffer_attachments(cecs_graphics_world *graphics_world, cecs_component_id component_id) {
    const cecs_component_storage_attachments *attachments = cecs_world_components_get_component_storage_attachments_unchecked(
        &graphics_world->world.components,
        component_id
    );
    assert(
        attachments->flags & cecs_component_storage_attachment_usage_graphics_buffer
        && "error: attachments of component must be flagged for graphics buffer usage"
    );
    return attachments->user_attachments;
}

cecs_buffer_storage_attachment *cecs_graphics_world_get_or_set_buffer_attachments(
    cecs_graphics_world *graphics_world,
    cecs_component_id component_id,
    cecs_buffer_storage_attachment *default_attachments
) {
    const cecs_component_storage_attachments *attachments = cecs_world_components_get_or_set_component_storage_attachments(
        &graphics_world->world.components,
        component_id,
        default_attachments,
        sizeof(cecs_buffer_storage_attachment),
        cecs_component_storage_attachment_usage_graphics_buffer
    );
    assert(
        attachments->flags & cecs_component_storage_attachment_usage_graphics_buffer
        && "error: attachments of component must be flagged for graphics buffer usage"
    );
    return attachments->user_attachments;
}

cecs_exclusive_index_buffer_pair cecs_graphics_world_get_index_buffers(cecs_graphics_world *graphics_world) {
    extern inline cecs_buffer_storage_attachment cecs_buffer_storage_attachment_uninitialized(size_t variant);

    cecs_buffer_storage_attachment uninitialized_buffer = cecs_buffer_storage_attachment_uninitialized(CECS_STREAM_STORAGE_INDEX_VARIANT);
    return (cecs_exclusive_index_buffer_pair){
        .u16 = &CECS_GRAPHICS_WORLD_GET_OR_SET_BUFFER_ATTACHMENTS(cecs_vertex_index_u16, graphics_world, &uninitialized_buffer)->buffer,
        .u32 = &CECS_GRAPHICS_WORLD_GET_OR_SET_BUFFER_ATTACHMENTS(cecs_vertex_index_u32, graphics_world, &uninitialized_buffer)->buffer
    };
}

static inline cecs_arena *cecs_world_default_buffer_arena(cecs_world *world) {
    return &world->resources.resources_arena;
}

cecs_arena *cecs_graphics_world_default_buffer_arena(cecs_graphics_world *graphics_world) {
    return cecs_world_default_buffer_arena(&graphics_world->world);
}

cecs_buffer_storage_attachment *cecs_graphics_world_get_or_init_uniform_buffer(
    cecs_graphics_world *world,
    cecs_graphics_context *context,
    cecs_component_id component_id,
    size_t size
) {
        assert(
        CECS_UNIFORM_IS_ALIGNED_SIZE(size)
        && "error: uniform size must be aligned to uniform buffer alignment"
        && CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT_VALUE
    );

    cecs_buffer_storage_attachment default_attachment = cecs_buffer_storage_attachment_create_uniform_uninitialized(
        (cecs_uniform_storage_attachment){
            .uniform_stride = size
        }
    );
    
    const cecs_component_storage_attachments *attachments = cecs_world_components_get_or_set_component_storage_attachments(
        &world->world.components,
        component_id,
        &default_attachment,
        sizeof(cecs_buffer_storage_attachment),
        cecs_component_storage_attachment_usage_graphics_buffer
    );
    assert(
        attachments->flags & cecs_component_storage_attachment_usage_graphics_buffer
        && "error: attachments of component must be flagged for graphics buffer usage"
    );
    cecs_buffer_storage_attachment *storage = attachments->user_attachments;

    if (!(storage->buffer_flags & cecs_buffer_flags_initialized)) {
        // TODO: check if storage is dense so that is shared
        cecs_buffer_storage_attachment_initialize(
            storage,
            context->device,
            cecs_graphics_world_default_buffer_arena(world),
            WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            size,
            cecs_webgpu_uniform_buffer_alignment
        );
    } else {
        assert(
            storage->buffer.alignment == cecs_webgpu_uniform_buffer_alignment
            && "fatal error: uniform buffer alignment mismatch"
        );
    }

    return storage;
}

cecs_entity_id cecs_world_add_entity_with_mesh(cecs_world *world, cecs_mesh *mesh) {
    cecs_entity_id entity = cecs_world_add_entity(world);
    CECS_WORLD_SET_COMPONENT(cecs_mesh, world, entity, mesh);
    return entity;
}
cecs_entity_id cecs_world_add_entity_with_indexed_mesh(cecs_world *world, cecs_mesh *mesh, cecs_index_stream *index_stream) {
    cecs_entity_id entity = cecs_world_add_entity(world);
    CECS_WORLD_SET_COMPONENT(cecs_mesh, world, entity, mesh);
    CECS_WORLD_SET_COMPONENT(cecs_index_stream, world, entity, index_stream);
    return entity;
}