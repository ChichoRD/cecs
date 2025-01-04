#include <memory.h>
#include <stdio.h>
#include "cecs_mesh_builder.h"

cecs_mesh_builder cecs_mesh_builder_create(cecs_world *graphics_world, cecs_mesh_builder_descriptor descriptor) {
    return (cecs_mesh_builder){
        .graphics_world = graphics_world,
        .descriptor = descriptor,
        .vertex_attribute_ids = cecs_sparse_set_create_of_integers_with_capacity(
            &graphics_world->resources.resources_arena,
            descriptor.vertex_attributes_expected_count,
            sizeof(cecs_component_id)
        ),
        .vertex_range = {0, 0},
        .index_range = {0, 0},
        .bounding_radius = 0.0f,
    };
}

static cecs_buffer_storage_attachment *cecs_mesh_builder_build_vertex_attribute(
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    cecs_vertex_attribute_id id
) {
    cecs_buffer_storage_attachment *storage = cecs_world_get_component_storage_attachments(
        builder->graphics_world,
        id
    );
    CECS_UNION_IS_ASSERT(cecs_vertex_attribute_storage_attachment, cecs_stream_storage_attachment, storage->stream);
    const cecs_vertex_attribute_storage_attachment *vertex_info =
        &CECS_UNION_GET_UNCHECKED(cecs_vertex_attribute_storage_attachment, storage->stream);

    if (!(storage->buffer_flags & cecs_buffer_flags_initialized)) {
        WGPUBufferDescriptor buffer_descriptor = {
            .label = "vertex buffer",
            .mappedAtCreation = false,
            .nextInChain = NULL,
            .size = vertex_info->max_vertex_count * vertex_info->attribute_size,
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc | WGPUBufferUsage_Vertex,
        };
        storage->buffer = cecs_dynamic_wgpu_buffer_create(context->device, &buffer_descriptor);
        storage->buffer_flags |= cecs_buffer_flags_initialized;
    }
    
    size_t vertex_count = cecs_exclusive_range_length(builder->vertex_range);
    size_t attribute_size = vertex_info->attribute_size;
    size_t required_size = vertex_count * attribute_size;

    void *attributes = NULL;
    assert(
        cecs_world_get_component_array(
            builder->graphics_world,
            builder->vertex_range,
            id,
            &attributes
        ) == vertex_count
        && "error: vertex attribute count mismatch"
    );
    cecs_dynamic_wgpu_buffer_upload(
        &storage->buffer,
        context->device,
        context->queue,
        builder->vertex_range.start * attribute_size,
        attributes,
        vertex_count * attribute_size
    );
    return storage;
}

static cecs_buffer_storage_attachment *cecs_mesh_builder_build_indices(
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    cecs_vertex_index_id id,
    size_t index_format_size
) {
    size_t index_count = cecs_exclusive_range_length(builder->index_range);
    assert(index_count > 0 && "error: trying to build zero indices");

    cecs_buffer_storage_attachment *storage = cecs_world_get_component_storage_attachments(
        builder->graphics_world,
        id
    );

    CECS_UNION_IS_ASSERT(cecs_index_storage_attachment, cecs_stream_storage_attachment, storage->stream);
    const cecs_index_storage_attachment *index_info =
        &CECS_UNION_GET_UNCHECKED(cecs_index_storage_attachment, storage->stream);
    assert(index_info->index_format == builder->descriptor.index_format && "error: index format mismatch");

    if (!(storage->buffer_flags & cecs_buffer_flags_initialized)) {
        WGPUBufferDescriptor buffer_descriptor = {
            .label = "index buffer",
            .mappedAtCreation = false,
            .nextInChain = NULL,
            .size = index_info->max_index_count * index_format_size,
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc | WGPUBufferUsage_Index,
        };
        storage->buffer = cecs_dynamic_wgpu_buffer_create(context->device, &buffer_descriptor);
        storage->buffer_flags |= cecs_buffer_flags_initialized;
    }

    void *indices = NULL;
    assert(
        cecs_world_get_component_array(
            builder->graphics_world,
            builder->index_range,
            id,
            &indices
        ) == index_count
        && "error: index count mismatch"
    );

    cecs_dynamic_wgpu_buffer_upload(
        &storage->buffer,
        context->device,
        context->queue,
        builder->index_range.start * index_format_size,
        indices,
        index_count * index_format_size
    );
    return storage;
}

cecs_mesh *cecs_mesh_builder_build_into(cecs_world *world, cecs_mesh_builder *builder, cecs_graphics_context *context) {
    size_t vertex_attributes_count = cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_vertex_attribute_id));
    cecs_vertex_attribute_reference *vertex_attribute_references = cecs_arena_alloc(
        &builder->graphics_world->resources.resources_arena,
        vertex_attributes_count * sizeof(cecs_vertex_attribute_reference)
    );
    for (size_t i = 0; i < vertex_attributes_count; i++) {
        cecs_vertex_attribute_id id = ((cecs_vertex_attribute_id *)cecs_sparse_set_data(&builder->vertex_attribute_ids))[i];
        cecs_buffer_storage_attachment *storage = cecs_mesh_builder_build_vertex_attribute(builder, context, id);

        const cecs_vertex_attribute_storage_attachment *vertex_info =
            &CECS_UNION_GET_UNCHECKED(cecs_vertex_attribute_storage_attachment, storage->stream);
        vertex_attribute_references[i] = (cecs_vertex_attribute_reference){
            .attribute_id = id,
            .stride = vertex_info->attribute_size,
        };
    }
    cecs_entity_id_range attribute_entities = cecs_world_add_entity_range(world, vertex_attributes_count);
    CECS_WORLD_SET_COMPONENT_ARRAY(
        cecs_vertex_attribute_reference,
        world,
        attribute_entities,
        vertex_attribute_references
    );

    size_t index_count = cecs_exclusive_range_length(builder->index_range);
    if (index_count > 0) {
        size_t index_format_size;
        cecs_vertex_index_id index_id;
        if (builder->descriptor.index_format == WGPUIndexFormat_Uint16) {
            index_format_size = sizeof(cecs_vertex_index_u16);
            index_id = CECS_COMPONENT_ID(cecs_vertex_index_u16);
        } else if (builder->descriptor.index_format == WGPUIndexFormat_Uint32) {
            index_format_size = sizeof(cecs_vertex_index_u32);
            index_id = CECS_COMPONENT_ID(cecs_vertex_index_u32);
        } else {
            assert(false && "fatal error: index format not set");
            exit(EXIT_FAILURE);
        }

        cecs_mesh_builder_build_indices(builder, context, index_id, index_format_size);
        CECS_WORLD_SET_COMPONENT(
            cecs_index_stream,
            world,
            builder->descriptor.mesh_id,
            (&(cecs_index_stream){
                .format = builder->descriptor.index_format,
                .index_count = index_count,
                .offset = builder->index_range.start * index_format_size,
            })
        );
    }

    return CECS_WORLD_SET_COMPONENT(
        cecs_mesh,
        world,
        builder->descriptor.mesh_id,
        (&(cecs_mesh){
            .vertex_entities = builder->vertex_range,
            .vertex_attribute_references = attribute_entities,
            .bounding_radius = builder->bounding_radius,
        })
    );
}

cecs_mesh_builder *cecs_mesh_builder_clear_vertex_attribute(cecs_mesh_builder *builder, cecs_vertex_attribute_id attribute_id) {
    cecs_vertex_attribute_id id;
    if (CECS_SPARSE_SET_REMOVE(
        cecs_component_id,
        &builder->vertex_attribute_ids,
        &builder->graphics_world->resources.resources_arena,
        attribute_id,
        &id
    )) {
        cecs_buffer_storage_attachment *storage = cecs_world_get_component_storage_attachments(
            builder->graphics_world,
            attribute_id
        );
        CECS_UNION_IS_ASSERT(cecs_vertex_attribute_storage_attachment, cecs_stream_storage_attachment, storage->stream);
        size_t *stored_vertex_count = &CECS_UNION_GET_UNCHECKED(cecs_vertex_attribute_storage_attachment, storage->stream).current_vertex_count;
        
        size_t vertex_count = cecs_exclusive_range_length(builder->vertex_range);
        assert(*stored_vertex_count >= vertex_count && "error: vertex count mismatch");

        *stored_vertex_count -= vertex_count;
        if (cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_component_id)) == 0) {
            cecs_world_remove_entity_range(builder->graphics_world, builder->vertex_range);
            builder->vertex_range = (cecs_entity_id_range){0, 0};
        } else {
            cecs_world_remove_component_array(
                builder->graphics_world,
                builder->vertex_range,
                attribute_id,
                builder->graphics_world->components.discard.handle
            );
        }
    }
    // else {
        // // TODO: maybe not err
        // assert(false && "error: vertex attribute not found");
    // }
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_set_vertex_attribute(
    cecs_mesh_builder *builder,
    cecs_vertex_attribute_id attribute_id,
    void *attributes_data,
    size_t attributes_count,
    size_t attribute_size
) {
    if (cecs_exclusive_range_is_empty(builder->vertex_range)) {
        builder->vertex_range = cecs_world_add_entity_range(builder->graphics_world, attributes_count);
    } else {
        assert(
            cecs_exclusive_range_length(builder->vertex_range) == attributes_count
            && "error: vertex attribute count mismatch; use cecs_mesh_builder_clear_vertex_attribute before setting new attributes"
        );
    }
    CECS_SPARSE_SET_SET(
        cecs_component_id,
        &builder->vertex_attribute_ids,
        &builder->graphics_world->resources.resources_arena,
        attribute_id,
        &attribute_id
    );

    cecs_vertex_attribute_storage_attachment *storage_vertex_info = cecs_world_get_or_set_component_storage_attachments(
        builder->graphics_world,
        attribute_id,
        &(cecs_buffer_storage_attachment){
            .stream = CECS_UNION_CREATE(
                cecs_vertex_attribute_storage_attachment,
                cecs_stream_storage_attachment,
                ((cecs_vertex_attribute_storage_attachment){
                    .current_vertex_count = 0,
                    .max_vertex_count = attributes_count,
                    .attribute_size = attribute_size,
                })
            ),
            .buffer = cecs_dynamic_wgpu_buffer_uninitialized(),
            .buffer_flags = cecs_buffer_flags_none
        },
        sizeof(cecs_buffer_storage_attachment)
    );
    assert(storage_vertex_info->attribute_size == attribute_size && "error: attribute size mismatch");
    storage_vertex_info->current_vertex_count += attributes_count;
    storage_vertex_info->max_vertex_count = max(storage_vertex_info->max_vertex_count, storage_vertex_info->current_vertex_count);

    cecs_world_set_component_array(
        builder->graphics_world,
        builder->vertex_range,
        attribute_id,
        attributes_data,
        attribute_size
    );
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_clear_indices(cecs_mesh_builder *builder) {
    size_t index_count = cecs_exclusive_range_length(builder->index_range);
    cecs_vertex_index_id index_id = 0;
    if (builder->descriptor.index_format == WGPUIndexFormat_Uint16) {
        index_id = CECS_COMPONENT_ID(cecs_vertex_index_u16);
    } else if (builder->descriptor.index_format == WGPUIndexFormat_Uint32) {
        index_id = CECS_COMPONENT_ID(cecs_vertex_index_u32);
    } else {
        assert(index_count == 0 && "fatal error: index format not set");
        return builder;
    }

    if (index_count != 0) {
        cecs_buffer_storage_attachment *storage_index_info = cecs_world_get_component_storage_attachments(
            builder->graphics_world,
            index_id
        );
        CECS_UNION_IS_ASSERT(cecs_index_storage_attachment, cecs_stream_storage_attachment, storage_index_info->stream);
        size_t *stored_index_count = &CECS_UNION_GET_UNCHECKED(cecs_index_storage_attachment, storage_index_info->stream).current_index_count;
        assert(*stored_index_count >= index_count && "error: index count mismatch");

        *stored_index_count -= index_count;

        cecs_world_remove_entity_range(builder->graphics_world, builder->index_range);
        builder->index_range = (cecs_entity_id_range){0, 0}; // TODO: index buffer
    }
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_set_indices(
    cecs_mesh_builder *builder,
    void *indices_data,
    size_t indices_count
) {
    if (cecs_exclusive_range_is_empty(builder->index_range)) {
        builder->index_range = cecs_world_add_entity_range(builder->graphics_world, indices_count);
    } else {
        assert(
            cecs_exclusive_range_length(builder->index_range) == indices_count
            && "error: index count mismatch; use cecs_mesh_builder_clear_indices before setting new indices"
        );
    }

    cecs_vertex_index_id index_id;
    switch (builder->descriptor.index_format) {
    case WGPUIndexFormat_Uint16: {
        index_id = CECS_COMPONENT_ID(cecs_vertex_index_u16);
        CECS_WORLD_SET_COMPONENT_ARRAY(
            cecs_vertex_index_u16,
            builder->graphics_world,
            builder->index_range,
            indices_data
        );
        break;
    }
    case WGPUIndexFormat_Uint32: {
        index_id = CECS_COMPONENT_ID(cecs_vertex_index_u32);
        CECS_WORLD_SET_COMPONENT_ARRAY(
            cecs_vertex_index_u32,
            builder->graphics_world,
            builder->index_range,
            indices_data
        );
        break;
    }
    default: {
        assert(false && "unreachable: invalid index format");
        exit(EXIT_FAILURE);
    }
    }

    cecs_index_storage_attachment *storage_index_info = cecs_world_get_or_set_component_storage_attachments(
        builder->graphics_world,
        index_id,
        &(cecs_buffer_storage_attachment){
            .stream = CECS_UNION_CREATE(
                cecs_index_storage_attachment,
                cecs_stream_storage_attachment,
                ((cecs_index_storage_attachment){
                    .current_index_count = 0,
                    .max_index_count = indices_count,
                    .index_format = builder->descriptor.index_format,
                })
            ),
            .buffer = cecs_dynamic_wgpu_buffer_uninitialized(),
            .buffer_flags = cecs_buffer_flags_none
        },
        sizeof(cecs_buffer_storage_attachment)
    );
    assert(storage_index_info->index_format == builder->descriptor.index_format && "error: index format mismatch");
    storage_index_info->current_index_count += indices_count;
    storage_index_info->max_index_count = max(storage_index_info->max_index_count, storage_index_info->current_index_count);

    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_clear(cecs_mesh_builder *builder) {
    size_t atribute_count = cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_component_id));
    cecs_component_id *remove_ids = calloc(
        atribute_count,
        sizeof(cecs_component_id)
    );
    memcpy(
        remove_ids,
        cecs_sparse_set_data(&builder->vertex_attribute_ids),
        atribute_count * sizeof(cecs_component_id)
    );
    for (size_t i = 0; i < atribute_count; i++) {
        cecs_mesh_builder_clear_vertex_attribute(builder, remove_ids[i]);
    }
    assert(
        cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_component_id)) == 0
        && "error: vertex attributes not cleared"
    );
    free(remove_ids);

    cecs_mesh_builder_clear_indices(builder);
    return builder;
}

cecs_mesh *cecs_mesh_builder_build_into_and_clear(cecs_world *world, cecs_mesh_builder *builder, cecs_graphics_context *context) {
    cecs_mesh *mesh = cecs_mesh_builder_build_into(world, builder, context);
    cecs_mesh_builder_clear(builder);
    return mesh;
}
