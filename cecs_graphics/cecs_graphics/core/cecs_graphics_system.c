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
    // TODO: for that, not only the gpu buffer must have adirty flag but cecs storage must also have
    // in order to dintinguish between just needing an upload or a stage and then upload
    (void)world;
    
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

bool cecs_graphics_system_sync_uniform_components_all(
    cecs_graphics_system *system,
    cecs_world *world,
    const cecs_component_id components[],
    size_t components_count,
    cecs_buffer_storage_attachment *out_uniform_buffers[]
) {
    bool all_found = true;
    size_t i = 0;
    while (i < components_count && all_found) {
        if (cecs_world_has_component_storage_attachments(&system->world.world, components[i])) {
            out_uniform_buffers[i] = cecs_graphics_system_sync_uniform_components(
                system,
                world,
                components[i]
            );
            ++i;
        } else {
            all_found = false;
        }
    }
    return all_found;
}

cecs_texture_reference *cecs_graphics_system_set_texture(
    cecs_graphics_system *system,
    cecs_world *world,
    const cecs_entity_id entity,
    const cecs_component_id texture_component_id,
    cecs_texture *texture
) {
    const cecs_relation_id texture_reference_id = CECS_RELATION_ID(cecs_texture_reference, texture_component_id);
    cecs_texture_reference *texture_reference;
    if (cecs_world_try_get_component(world, entity, texture_reference_id, &texture_reference)) {
        cecs_texture *old_texture = CECS_WORLD_GET_COMPONENT(cecs_texture, &system->world.world, texture_reference->texture_id);
        wgpuTextureViewRelease(old_texture->texture_view);
        *old_texture = *texture;
    } else {
        cecs_entity_id texture_id = cecs_world_add_entity(&system->world.world);
        CECS_WORLD_SET_COMPONENT(
            cecs_texture,
            &system->world.world,
            texture_id,
            texture
        );
        texture_reference = CECS_WORLD_SET_COMPONENT_RELATION(
            cecs_texture_reference,
            world,
            entity,
            &(cecs_texture_reference){texture_id},
            texture_component_id
        );
    }
    return texture_reference;
}
