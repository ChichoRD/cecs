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
        cecs_sized_component_storage storage = cecs_world_components_iterator_current(&it);
        if (cecs_world_has_component_storage_attachments(&w->world, storage.component_id)) {
            cecs_buffer_storage_attachment *buffer_attachment = cecs_world_get_component_storage_attachments(
                &w->world,
                storage.component_id
            );
            
            if (buffer_attachment->buffer_flags & cecs_buffer_flags_initialized) {
                cecs_buffer_storage_attachment_free(buffer_attachment);
            }
        }
    }
    cecs_world_free(&w->world);
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

CECS_COMPONENT_DEFINE(cecs_uniform_raw_stream);

static cecs_buffer_storage_attachment *cecs_world_get_or_init_uniform_buffer(
    cecs_world *world,
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
    // FIXME: prevent or share with the user storage attachments
    cecs_buffer_storage_attachment *storage = cecs_world_get_or_set_component_storage_attachments(
        world,
        component_id,
        &default_attachment,
        sizeof(cecs_buffer_storage_attachment)
    );

    if (!(storage->buffer_flags & cecs_buffer_flags_initialized)) {
        // TODO: check if storage is dense so that is shared
        cecs_buffer_storage_attachment_initialize(
            storage,
            context->device,
            cecs_world_default_buffer_arena(world),
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

const cecs_uniform_raw_stream *cecs_world_set_component_as_uniform(
    cecs_world *world,
    cecs_graphics_context *context,
    cecs_entity_id entity,
    cecs_component_id component_id,
    void *component,
    size_t size
) {
    cecs_buffer_storage_attachment *storage = cecs_world_get_or_init_uniform_buffer(
        world,
        context,
        component_id,
        size
    );
    cecs_uniform_raw_stream stream;
    if (cecs_dynamic_wgpu_buffer_is_shared(&storage->buffer)) {
        cecs_world_set_component(
            world,
            entity,
            component_id,
            component,
            size
        );
        stream = (cecs_uniform_raw_stream){
            .offset = cecs_dynamic_wgpu_buffer_get_offset(&storage->buffer, entity),
            .size = size
        };
        storage->buffer_flags |= cecs_buffer_flags_dirty;
    } else {
        cecs_world_set_component(
            world,
            entity,
            component_id,
            component,
            size
        );
        stream = (cecs_uniform_raw_stream){
            .offset = cecs_dynamic_wgpu_buffer_stage(
                &storage->buffer,
                cecs_world_default_buffer_arena(world),
                entity * size,
                component,
                size
            ),
            .size = size
        };
        storage->buffer_flags |= cecs_buffer_flags_dirty;
    }

    return CECS_WORLD_SET_COMPONENT_RELATION(
        cecs_uniform_raw_stream,
        world,
        entity,
        &stream,
        component_id
    );
}

cecs_buffer_storage_attachment *cecs_world_sync_uniform_components(
    cecs_world *world,
    cecs_graphics_context *context,
    cecs_component_id component_id
) {
    cecs_buffer_storage_attachment *storage = cecs_world_get_component_storage_attachments(
        world,
        component_id
    );
    // TODO: maybe iter and update the cecs_uniform_raw_stream of those that have this component

    assert(
        storage->buffer_flags & cecs_buffer_flags_initialized
        && "error: uniform buffer must be initialized"
    );
    if (storage->buffer_flags & cecs_buffer_flags_dirty) {
        cecs_dynamic_wgpu_buffer_upload_all(
            &storage->buffer,
            context->device,
            context->queue,
            cecs_world_default_buffer_arena(world)
        );
        storage->buffer_flags &= ~cecs_buffer_flags_dirty;
    }
    return storage;
}
