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

CECS_COMPONENT_DEFINE(cecs_uniform_raw_stream);
const cecs_uniform_raw_stream *cecs_graphics_system_set_component_as_uniform(
    cecs_graphics_system *system,
    cecs_world *world,
    cecs_entity_id entity,
    cecs_component_id component_id,
    void *component,
    size_t size
) {
    cecs_buffer_storage_attachment *storage = cecs_graphics_world_get_or_init_uniform_buffer(
        &system->world,
        &system->context,
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
                cecs_graphics_world_default_buffer_arena(&system->world),
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

cecs_buffer_storage_attachment *cecs_graphics_system_sync_uniform_components(
    cecs_graphics_system *system,
    cecs_world *world,
    cecs_component_id component_id
) {
    cecs_buffer_storage_attachment *storage = cecs_graphics_world_get_buffer_attachments(
        &system->world,
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
            system->context.device,
            system->context.queue,
            cecs_graphics_world_default_buffer_arena(&system->world)
        );
        storage->buffer_flags &= ~cecs_buffer_flags_dirty;
    }
    return storage;
}
