#include "cecs_graphics_world.h"

cecs_graphics_world cecs_graphics_world_create(size_t vertex_capacity, size_t vertex_attributes_capacity) {
    return (cecs_graphics_world){
        .world = cecs_world_create(
            vertex_capacity,
            vertex_attributes_capacity,
            1
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
        cecs_buffer_storage_attachment *buffer_attachment = cecs_world_get_component_storage_attachments(
            &w->world,
            storage.component_id
        );
        if (buffer_attachment->buffer_flags & cecs_buffer_flags_initialized) {
            cecs_dynamic_wgpu_buffer_free(&buffer_attachment->buffer);
        }
    }
    cecs_world_free(&w->world);
}

cecs_exclusive_index_buffer_pair cecs_graphics_world_get_index_buffers(cecs_graphics_world *graphics_world) {
    cecs_buffer_storage_attachment uninitialized_buffer = cecs_buffer_storage_attachment_uninitialized();
    return (cecs_exclusive_index_buffer_pair){
        .u16 = &CECS_GRAPHICS_WORLD_GET_OR_SET_BUFFER_ATTACHMENTS(cecs_vertex_index_u16, graphics_world, &uninitialized_buffer)->buffer,
        .u32 = &CECS_GRAPHICS_WORLD_GET_OR_SET_BUFFER_ATTACHMENTS(cecs_vertex_index_u32, graphics_world, &uninitialized_buffer)->buffer
    };
}