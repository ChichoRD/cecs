#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "cecs_mesh_builder.h"

cecs_mesh_builder cecs_mesh_builder_create(cecs_graphics_world *graphics_world, cecs_mesh_builder_descriptor descriptor, cecs_arena *builder_arena) {
    return (cecs_mesh_builder){
        .graphics_world = graphics_world,
        .descriptor = descriptor,
        .builder_arena = builder_arena,
        .vertex_attribute_ids = cecs_sparse_set_create_of_integers_with_capacity(
            builder_arena,
            descriptor.vertex_attributes_expected_count,
            sizeof(cecs_component_id)
        ),
        .vertex_range = {0, 0},
        .index_range = {0, 0},
        .bounding_radius = 0.0f,
    };
}

cecs_mesh_builder cecs_mesh_builder_create_from(const cecs_mesh_builder *builder, cecs_mesh_builder_descriptor descriptor) {
    return cecs_mesh_builder_create(builder->graphics_world, descriptor, builder->builder_arena);
}

static cecs_buffer_storage_attachment *cecs_mesh_builder_build_vertex_attribute(
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    cecs_vertex_attribute_id id
) {
    cecs_buffer_storage_attachment *storage = cecs_world_get_component_storage_attachments(
        &builder->graphics_world->world,
        id
    );
    CECS_UNION_IS_ASSERT(cecs_vertex_storage_attachment, cecs_stream_storage_attachment, storage->stream);
    const cecs_vertex_storage_attachment *vertex_info =
        &CECS_UNION_GET_UNCHECKED(cecs_vertex_storage_attachment, storage->stream);

    if (!(storage->buffer_flags & cecs_buffer_flags_initialized)) {
        uint64_t size = vertex_info->max_vertex_count * vertex_info->attribute_size;
        WGPUBufferUsageFlags usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
        cecs_buffer_storage_attachment_initialize(
            storage, context->device, cecs_graphics_world_default_buffer_arena(builder->graphics_world), usage, size, cecs_webgpu_copy_buffer_alignment
        );
    }
    
    const size_t vertex_count = cecs_exclusive_range_length(builder->vertex_range);
    const cecs_vertex_stream vertex_stream = cecs_vertex_stream_create(
        builder->vertex_range.start,
        vertex_count,
        vertex_info->attribute_size
    );

    void *attributes = NULL;
    assert(
        cecs_world_get_component_array(
            &builder->graphics_world->world,
            builder->vertex_range,
            id,
            &attributes
        ) == vertex_count
        && "error: vertex attribute count mismatch"
    );
    // TODO: use function to lower the key (offset) instead of indexing bytes
    cecs_dynamic_wgpu_buffer_upload(
        &storage->buffer,
        context->device,
        context->queue,
        cecs_graphics_world_default_buffer_arena(builder->graphics_world),
        vertex_stream.offset,
        attributes,
        vertex_stream.size
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
        &builder->graphics_world->world,
        id
    );

    CECS_UNION_IS_ASSERT(cecs_index_storage_attachment, cecs_stream_storage_attachment, storage->stream);
    const cecs_index_storage_attachment *index_info =
        &CECS_UNION_GET_UNCHECKED(cecs_index_storage_attachment, storage->stream);
    assert(index_info->index_format == builder->descriptor.index_format && "error: index format mismatch");

    if (!(storage->buffer_flags & cecs_buffer_flags_initialized)) {
        uint64_t size = index_info->max_index_count * index_format_size;
        WGPUBufferUsageFlags usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
        cecs_buffer_storage_attachment_initialize(
            storage, context->device, cecs_graphics_world_default_buffer_arena(builder->graphics_world), usage, size, cecs_webgpu_copy_buffer_alignment
        );
    }

    void *indices = NULL;
    assert(
        cecs_world_get_component_array(
            &builder->graphics_world->world,
            builder->index_range,
            id,
            &indices
        ) == index_count
        && "error: index count mismatch"
    );

    const cecs_buffer_stream index_stream = cecs_buffer_stream_from_index_size(
        (cecs_index_stream){
            .format = builder->descriptor.index_format,
            .index_count = index_count,
            .first_index = builder->index_range.start,
        },
        index_format_size
    );
    cecs_dynamic_wgpu_buffer_upload(
        &storage->buffer,
        context->device,
        context->queue,
        cecs_graphics_world_default_buffer_arena(builder->graphics_world),
        index_stream.offset,
        indices,
        index_stream.size
    );
    return storage;
}

cecs_mesh cecs_mesh_builder_build_into(
    cecs_world *world,
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    cecs_index_stream *out_index_stream
) {
    size_t vertex_attributes_count = cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_vertex_attribute_id));
    cecs_vertex_attribute_reference *vertex_attribute_references = calloc(
        vertex_attributes_count,
        sizeof(cecs_vertex_attribute_reference)
    );
    for (size_t i = 0; i < vertex_attributes_count; i++) {
        cecs_vertex_attribute_id id = ((cecs_vertex_attribute_id *)cecs_sparse_set_values(&builder->vertex_attribute_ids))[i];
        cecs_buffer_storage_attachment *storage = cecs_mesh_builder_build_vertex_attribute(builder, context, id);

        const cecs_vertex_storage_attachment *vertex_info =
            &CECS_UNION_GET_UNCHECKED(cecs_vertex_storage_attachment, storage->stream);
        vertex_attribute_references[i] = (cecs_vertex_attribute_reference){
            .attribute_id = id,
            .stride = vertex_info->attribute_size,
        };
        // TODO: maybe add too an array of cecs_vertex_stream
    }
    cecs_entity_id_range attribute_entities = cecs_world_add_entity_range(world, vertex_attributes_count);
    CECS_WORLD_SET_COMPONENT_ARRAY(
        cecs_vertex_attribute_reference,
        world,
        attribute_entities,
        vertex_attribute_references
    );
    free(vertex_attribute_references);

    size_t index_count = cecs_exclusive_range_length(builder->index_range);
    bool has_index_stream = index_count > 0;
    assert((out_index_stream != NULL) == has_index_stream && "error: index stream - out_component mismatch");
    if (has_index_stream) {
        cecs_index_format_info format_info = cecs_index_format_info_from(builder->descriptor.index_format);

        cecs_mesh_builder_build_indices(builder, context, format_info.id, format_info.size);
        *out_index_stream = (cecs_index_stream){
            .format = builder->descriptor.index_format,
            .index_count = index_count,
            .first_index = builder->index_range.start,
        };
    }

    return (cecs_mesh){
        .vertex_entities = builder->vertex_range,
        .vertex_attribute_references = attribute_entities,
        .bounding_radius = builder->bounding_radius,
    };
}

bool cecs_mesh_builder_is_clear(const cecs_mesh_builder *builder) {
    bool clear = cecs_exclusive_range_is_empty(builder->vertex_range)
        && cecs_exclusive_range_is_empty(builder->index_range);
    assert(
        (!clear || cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_vertex_attribute_id)))
        && "fatal error: builder clear range and attributes mismatch"
    );
    return clear;
}

cecs_mesh_builder *cecs_mesh_builder_clear_vertex_attribute(cecs_mesh_builder *builder, cecs_vertex_attribute_id attribute_id)
{
    cecs_vertex_attribute_id id;
    if (CECS_SPARSE_SET_REMOVE(
        cecs_component_id,
        &builder->vertex_attribute_ids,
        builder->builder_arena,
        attribute_id,
        &id
    )) {
        cecs_buffer_storage_attachment *storage = cecs_world_get_component_storage_attachments(
            &builder->graphics_world->world,
            attribute_id
        );
        CECS_UNION_IS_ASSERT(cecs_vertex_storage_attachment, cecs_stream_storage_attachment, storage->stream);
        cecs_vertex_storage_attachment *vertex_info =
            &CECS_UNION_GET_UNCHECKED(cecs_vertex_storage_attachment, storage->stream);
        
        size_t vertex_count = cecs_exclusive_range_length(builder->vertex_range);
        assert(vertex_info->current_vertex_count >= vertex_count && "error: vertex count mismatch");

        vertex_info->current_vertex_count -= vertex_count;
        if (cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_component_id)) == 0) {
            cecs_world_remove_entity_range(&builder->graphics_world->world, builder->vertex_range);
            builder->vertex_range = (cecs_entity_id_range){0, 0};
        } else {
            cecs_world_remove_component_array(
                &builder->graphics_world->world,
                builder->vertex_range,
                attribute_id,
                cecs_world_use_component_discard(
                    &builder->graphics_world->world,
                    vertex_count * vertex_info->attribute_size
                )
            );
        }
    }
    
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
        builder->vertex_range = cecs_world_add_entity_range(&builder->graphics_world->world, attributes_count);
    } else {
        assert(
            (size_t)cecs_exclusive_range_length(builder->vertex_range) == attributes_count
            && "error: vertex attribute count mismatch; use cecs_mesh_builder_clear_vertex_attribute before setting new attributes"
        );
    }
    
    CECS_SPARSE_SET_SET(
        cecs_component_id,
        &builder->vertex_attribute_ids,
        builder->builder_arena,
        attribute_id,
        &attribute_id
    );

    cecs_buffer_storage_attachment default_attachment = cecs_buffer_storage_attachment_create_vertex_uninitialized(
        (cecs_vertex_storage_attachment){
            .max_vertex_count = attributes_count,
            .current_vertex_count = 0,
            .attribute_size = attribute_size,
        }
    );
    cecs_vertex_storage_attachment *storage_vertex_info = cecs_world_get_or_set_component_storage_attachments(
        &builder->graphics_world->world,
        attribute_id,
        &default_attachment,
        sizeof(cecs_buffer_storage_attachment)
    );
    assert(storage_vertex_info->attribute_size == attribute_size && "error: attribute size mismatch");
    storage_vertex_info->current_vertex_count += attributes_count;
    storage_vertex_info->max_vertex_count = max(storage_vertex_info->max_vertex_count, storage_vertex_info->current_vertex_count);

    cecs_world_set_component_array(
        &builder->graphics_world->world,
        builder->vertex_range,
        attribute_id,
        attributes_data,
        attribute_size
    );
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_clear_indices(cecs_mesh_builder *builder) {
    size_t index_count = cecs_exclusive_range_length(builder->index_range);
    cecs_vertex_index_id index_id = cecs_index_format_info_from(builder->descriptor.index_format).id;

    if (index_count > 0) {
        cecs_buffer_storage_attachment *storage_index_info = cecs_world_get_component_storage_attachments(
            &builder->graphics_world->world,
            index_id
        );
        CECS_UNION_IS_ASSERT(cecs_index_storage_attachment, cecs_stream_storage_attachment, storage_index_info->stream);
        size_t *stored_index_count = &CECS_UNION_GET_UNCHECKED(cecs_index_storage_attachment, storage_index_info->stream).current_index_count;
        assert(*stored_index_count >= index_count && "error: index count mismatch");

        *stored_index_count -= index_count;

        cecs_world_remove_entity_range(&builder->graphics_world->world, builder->index_range);
        builder->index_range = (cecs_entity_id_range){0, 0};
    }
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_set_indices(
    cecs_mesh_builder *builder,
    void *indices_data,
    size_t indices_count
) {
    if (cecs_exclusive_range_is_empty(builder->index_range)) {
        builder->index_range = cecs_world_add_entity_range(&builder->graphics_world->world, indices_count);
    } else {
        assert(
            (size_t)cecs_exclusive_range_length(builder->index_range) == indices_count
            && "error: index count mismatch; use cecs_mesh_builder_clear_indices before setting new indices"
        );
    }

    cecs_index_format_info format_info = cecs_index_format_info_from(builder->descriptor.index_format);
    cecs_buffer_storage_attachment default_attachment = cecs_buffer_storage_attachment_create_index_uninitialized(
        (cecs_index_storage_attachment){
            .current_index_count = 0,
            .max_index_count = indices_count,
            .index_format = builder->descriptor.index_format,
        }
    );
    cecs_index_storage_attachment *storage_index_info = cecs_world_get_or_set_component_storage_attachments(
        &builder->graphics_world->world,
        format_info.id,
        &default_attachment,
        sizeof(cecs_buffer_storage_attachment)
    );
    assert(storage_index_info->index_format == builder->descriptor.index_format && "error: index format mismatch");
    storage_index_info->current_index_count += indices_count;
    storage_index_info->max_index_count = max(storage_index_info->max_index_count, storage_index_info->current_index_count);

    cecs_world_set_component_array(
        &builder->graphics_world->world,
        builder->index_range,
        format_info.id,
        indices_data,
        format_info.size
    );
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
        cecs_sparse_set_values(&builder->vertex_attribute_ids),
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

cecs_mesh cecs_mesh_builder_build_into_and_clear(
    cecs_world *world,
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    cecs_index_stream *out_index_stream
) {
    cecs_mesh mesh = cecs_mesh_builder_build_into(world, builder, context, out_index_stream);
    cecs_mesh_builder_clear(builder);
    return mesh;
}
