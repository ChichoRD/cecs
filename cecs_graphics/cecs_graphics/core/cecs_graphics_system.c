#include <memory.h>
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
    cecs_buffer_flags previous_flags;
    cecs_buffer_storage_attachment *storage = cecs_graphics_world_get_or_init_uniform_buffer(
        &system->world,
        &system->context,
        component_id,
        size,
        &previous_flags
    );
    assert(
        (storage->buffer_flags & cecs_buffer_status_initialized)
        && "error: uniform buffer must be initialized"
    );
    assert(
        (storage->buffer_flags & cecs_buffer_type_uniform)
        && "error: buffer must be a uniform buffer"
    );
    assert(
        (storage->buffer_flags & cecs_buffer_type_dynamic_element)
        && "error: buffer must be a dynamic buffer"
    );

    cecs_dynamic_wgpu_element_buffer *buffer = &storage->buffer.element_buffer;
    if (!(previous_flags & cecs_buffer_status_initialized)) {
        storage->offsets.element_offset = entity;
    } else if (entity < storage->offsets.element_offset) {
        const size_t elements_expansion = storage->offsets.element_offset - entity;
        cecs_buffer_storage_attachment_extend_dynamic_elements(
            storage,
            cecs_graphics_world_default_buffer_arena(&system->world),
            0,
            elements_expansion
        );
        storage->offsets.element_offset = entity;
    }

    cecs_world_set_component(
        world,
        entity,
        component_id,
        component,
        size
    );
    
    cecs_uniform_raw_stream stream = (cecs_uniform_raw_stream){
        .offset = cecs_dynamic_wgpu_element_buffer_stage(
            buffer,
            cecs_graphics_world_default_buffer_arena(&system->world),
            entity - storage->offsets.element_offset,
            0,
            component,
            size
        ),
        .size = size
    };
    storage->buffer_flags |= cecs_buffer_status_dirty;


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
    cecs_arena *sync_arena,
    const cecs_component_id component_id
) {
    cecs_sized_component_storage *storage = cecs_world_components_get_component_storage_expect(
        &world->components,
        component_id
    );
    cecs_buffer_storage_attachment *uniform_buffer = cecs_graphics_world_get_buffer_attachments(
        &system->world,
        component_id
    );
    assert(
        (uniform_buffer->buffer_flags & cecs_buffer_status_initialized)
        && "error: uniform buffer must be initialized"
    );
    assert(
        (uniform_buffer->buffer_flags & cecs_buffer_type_uniform)
        && "error: buffer must be a uniform buffer"
    );
    assert(
        (uniform_buffer->buffer_flags & cecs_buffer_type_dynamic_element)
        && "error: buffer must be a dynamic buffer"
    );

    if (storage->storage.status & cecs_component_storage_status_dirty) {
        uint8_t *stage = uniform_buffer->buffer.element_buffer.buffer.stage;

        CECS_COMPONENT_ITERATION_HANDLE_STRUCT(cecs_uniform_raw_stream, void) handle;
        cecs_component_iterator it = CECS_COMPONENT_ITERATOR_CREATE_GROUPPED(&world->components, sync_arena,
            CECS_COMPONENT_GROUP_FROM_IDS(cecs_component_access_inmmutable, cecs_component_group_search_all,
                CECS_RELATION_ID(cecs_uniform_raw_stream, component_id), component_id
            )
        );
        for (
            cecs_component_iterator_begin_iter(&it, sync_arena);
            !cecs_component_iterator_done(&it);
            cecs_component_iterator_next(&it)
        ) {
            cecs_component_iterator_current(&it, (void **)&handle);
            memcpy(
                stage + handle.cecs_uniform_raw_stream_component->offset,
                handle.void_component,
                handle.cecs_uniform_raw_stream_component->size
            );
        }
        cecs_component_iterator_end_iter(&it);

        // HACK: we are the ONES removing the dirty flag
        storage->storage.status &= ~cecs_component_storage_status_dirty;
        uniform_buffer->buffer_flags |= cecs_buffer_status_dirty;
    }
    
    assert(
        uniform_buffer->buffer_flags & cecs_buffer_status_initialized
        && "error: uniform buffer must be initialized"
    );
    if (uniform_buffer->buffer_flags & cecs_buffer_status_dirty) {
        cecs_dynamic_wgpu_element_buffer_upload_all(
            &uniform_buffer->buffer.element_buffer,
            system->context.device,
            system->context.queue,
            cecs_graphics_world_default_buffer_arena(&system->world)
        );
        uniform_buffer->buffer_flags &= ~cecs_buffer_status_dirty;
    }
    return uniform_buffer;
}

bool cecs_graphics_system_sync_uniform_components_all(
    cecs_graphics_system *system,
    cecs_world *world,
    cecs_arena *sync_arena,
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
                sync_arena,
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
    if (cecs_world_try_get_component(world, entity, texture_reference_id, (void **)&texture_reference)) {
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
