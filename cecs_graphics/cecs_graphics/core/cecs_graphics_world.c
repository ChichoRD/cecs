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

cecs_arena *cecs_graphics_world_buffer_arena(cecs_graphics_world *graphics_world) {
    return &graphics_world->world.resources.resources_arena;
}
