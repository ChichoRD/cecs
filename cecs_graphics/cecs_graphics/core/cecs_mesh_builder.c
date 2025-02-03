#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "cecs_mesh_builder.h"

cecs_buffer_attribute_builder cecs_attribute_builder_create(cecs_graphics_world *graphics_world, cecs_arena *builder_arena, const size_t expected_attribute_count) {
    return (cecs_buffer_attribute_builder){
        .graphics_world = graphics_world,
        .builder_arena = builder_arena,
        .attribute_ids = cecs_sparse_set_create_of_integers_with_capacity(
            builder_arena,
            expected_attribute_count,
            sizeof(cecs_buffer_attribute_id)
        ),
        .attribute_range = {0},
    };
}

cecs_buffer_attribute_builder *cecs_attribute_builder_clear_attribute(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
) {
    cecs_buffer_attribute_id id;
    if (CECS_SPARSE_SET_REMOVE(
        cecs_buffer_attribute_id,
        &builder->attribute_ids,
        builder->builder_arena,
        attribute_id,
        &id
    )) {
        const cecs_buffer_attribute_builder_buffer_info info = get_buffer_info(builder, attribute_id, userdata);
        size_t attribute_count = cecs_exclusive_range_length(builder->attribute_range);
        assert((*info.attribute_count) >= attribute_count && "error: attribute count mismatch");
        
        (*info.attribute_count) -= attribute_count;
        if (cecs_sparse_set_count_of_size(&builder->attribute_ids, sizeof(cecs_buffer_attribute_id)) == 0) {
            cecs_world_remove_entity_range(&builder->graphics_world->world, builder->attribute_range);
            builder->attribute_range = (cecs_entity_id_range){0};
        } else {
            cecs_world_remove_component_array(
                &builder->graphics_world->world,
                builder->attribute_range,
                attribute_id,
                cecs_world_use_component_discard(
                    &builder->graphics_world->world,
                    attribute_count * (*info.attribute_stride)
                )
            );
        }
    }
    
    return builder;
}

cecs_buffer_attribute_builder *cecs_attribute_builder_set_attribute(
    cecs_buffer_attribute_builder *builder,
    cecs_buffer_attribute_id attribute_id,
    void *attributes_data,
    const size_t attributes_count,
    const size_t attribute_size,
    cecs_attribute_builder_get_buffer_info_init *const get_buffer_info_init,
    void *userdata
) {
    if (cecs_exclusive_range_is_empty(builder->attribute_range)) {
        builder->attribute_range = cecs_world_add_entity_range(&builder->graphics_world->world, attributes_count);
    } else {
        assert(
            (size_t)cecs_exclusive_range_length(builder->attribute_range) == attributes_count
            && "error: attribute count mismatch; use cecs_attribute_builder_clear_attribute before setting new attributes"
        );
    }
    
    CECS_SPARSE_SET_SET(
        cecs_component_id,
        &builder->attribute_ids,
        builder->builder_arena,
        attribute_id,
        &attribute_id
    );

    const cecs_buffer_attribute_builder_buffer_info info = get_buffer_info_init(builder, attribute_id, attributes_count, attribute_size, userdata);
    assert((*info.attribute_stride) == attribute_size && "error: attribute size mismatch");
    (*info.attribute_count) += attributes_count;
    (*info.max_attribute_count) = max((*info.max_attribute_count), (*info.attribute_count));

    cecs_world_set_component_array(
        &builder->graphics_world->world,
        builder->attribute_range,
        attribute_id,
        attributes_data,
        attribute_size
    );
    return builder;
}

bool cecs_attribute_builder_is_clear(const cecs_buffer_attribute_builder *builder) {
    bool clear = cecs_exclusive_range_is_empty(builder->attribute_range);
    assert(
        (!clear || cecs_sparse_set_count_of_size(&builder->attribute_ids, sizeof(cecs_buffer_attribute_id)))
        && "fatal error: builder clear range and attributes mismatch"
    );
    return clear;
}

cecs_buffer_attribute_builder *cecs_attribute_builder_clear(
    cecs_buffer_attribute_builder *builder,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
) {
    size_t atribute_count = cecs_sparse_set_count_of_size(&builder->attribute_ids, sizeof(cecs_buffer_attribute_id));
    cecs_component_id *remove_ids = cecs_arena_alloc(builder->builder_arena, atribute_count * sizeof(cecs_buffer_attribute_id));
    memcpy(
        remove_ids,
        cecs_sparse_set_values(&builder->attribute_ids),
        atribute_count * sizeof(cecs_buffer_attribute_id)
    );
    for (size_t i = 0; i < atribute_count; i++) {
        cecs_attribute_builder_clear_attribute(builder, remove_ids[i], get_buffer_info, userdata);
    }
    assert(
        cecs_sparse_set_count_of_size(&builder->attribute_ids, sizeof(cecs_buffer_attribute_id)) == 0
        && "error: vertex attributes not cleared"
    );
    return builder;
}

cecs_buffer_attribute_range cecs_attribute_builder_build_into_and_clear(
    cecs_world *world,
    cecs_buffer_attribute_builder *builder,
    cecs_graphics_context *context,
    const WGPUBufferUsageFlags usage,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
) {
    cecs_buffer_attribute_range range = cecs_attribute_builder_build_into(world, builder, context, usage, get_buffer_info, userdata);
    cecs_attribute_builder_clear(builder, get_buffer_info, userdata);
    return range;
}

static cecs_buffer_storage_attachment *cecs_attribute_builder_build_attribute(
    cecs_buffer_attribute_builder *builder,
    cecs_graphics_context *context,
    const cecs_buffer_attribute_id attribute_id,
    const WGPUBufferUsageFlags usage,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
) {
    cecs_buffer_storage_attachment *storage = cecs_graphics_world_get_buffer_attachments(
        builder->graphics_world,
        attribute_id
    );

    const cecs_buffer_attribute_builder_buffer_info info = get_buffer_info(builder, attribute_id, userdata);
    if (!(storage->buffer_flags & cecs_buffer_flags_initialized)) {
        uint64_t size = (*info.max_attribute_count) * (*info.attribute_stride);
        cecs_buffer_storage_attachment_initialize(
            storage, context->device, cecs_graphics_world_default_buffer_arena(builder->graphics_world), usage, size, cecs_webgpu_copy_buffer_alignment
        );
    }
    
    const size_t attribute_count = cecs_exclusive_range_length(builder->attribute_range);
    const cecs_buffer_stream attribute_stream = cecs_buffer_stream_create(
        builder->attribute_range.start,
        attribute_count,
        *info.attribute_stride
    );

    void *attributes = NULL;
    assert(
        cecs_world_get_component_array(
            &builder->graphics_world->world,
            builder->attribute_range,
            attribute_id,
            &attributes
        ) == attribute_count
        && "error: vertex attribute count mismatch"
    );

    // TODO: use function to lower the key (offset) instead of indexing bytes
    cecs_dynamic_wgpu_buffer_stage_and_upload(
        &storage->buffer,
        context->device,
        context->queue,
        cecs_graphics_world_default_buffer_arena(builder->graphics_world),
        attribute_stream.offset,
        attributes,
        attribute_stream.size
    );
    return storage;
}

cecs_buffer_attribute_range cecs_attribute_builder_build_into(
    cecs_world *world,
    cecs_buffer_attribute_builder *builder,
    cecs_graphics_context *context,
    const WGPUBufferUsageFlags usage,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
) {
    size_t attributes_count = cecs_sparse_set_count_of_size(&builder->attribute_ids, sizeof(cecs_buffer_attribute_id));
    if (attributes_count == 0) {
        return (cecs_buffer_attribute_range){{0}, {0}};
    }

    cecs_buffer_attribute_reference *buffer_attribute_references = cecs_arena_alloc(
        builder->builder_arena,
        attributes_count * sizeof(cecs_buffer_attribute_reference) 
    );
    for (size_t i = 0; i < attributes_count; i++) {
        cecs_buffer_attribute_id id = ((cecs_buffer_attribute_id *)cecs_sparse_set_values(&builder->attribute_ids))[i];
        cecs_buffer_storage_attachment *storage = cecs_attribute_builder_build_attribute(builder, context, id, usage, get_buffer_info, userdata);

        const cecs_attribute_storage_attachment *attributes_info =
            &CECS_UNION_GET_UNCHECKED(cecs_vertex_storage_attachment, storage->stream);
        buffer_attribute_references[i] = (cecs_buffer_attribute_reference){
            .attribute_id = id,
            .stride = attributes_info->attribute_stride,
        };
        // TODO: maybe add too an array of cecs_vertex_stream
    }
    cecs_entity_id_range attribute_entities = cecs_world_add_entity_range(world, attributes_count);
    CECS_WORLD_SET_COMPONENT_ARRAY(
        cecs_buffer_attribute_reference,
        world,
        attribute_entities,
        buffer_attribute_references
    );

    return (cecs_buffer_attribute_range){
        .attribute_entities = builder->attribute_range,
        .attribute_references = attribute_entities,
    };
}

static cecs_buffer_attribute_builder_buffer_info cecs_mesh_builder_get_vertex_info_init(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    const size_t attributes_count,
    const size_t attribute_size,
    void *userdata
) {
    assert(userdata == NULL && "error: userdata not expected");
    (void)userdata;
    
    cecs_buffer_storage_attachment default_attachment = cecs_buffer_storage_attachment_create_vertex_uninitialized((cecs_attribute_storage_attachment){
        .max_attribute_count = attributes_count,
        .current_attribute_count = 0,
        .attribute_stride = attribute_size,
    });
    cecs_vertex_storage_attachment *storage_vertex_info = cecs_graphics_world_get_or_set_vertex_buffer_attachments(
        builder->graphics_world,
        attribute_id,
        &default_attachment
    );

    assert(storage_vertex_info->attribute_stride == attribute_size && "error: attribute size mismatch");
    return (cecs_buffer_attribute_builder_buffer_info){
        .attribute_count = &storage_vertex_info->current_attribute_count,
        .attribute_stride = &storage_vertex_info->attribute_stride,
        .max_attribute_count = &storage_vertex_info->max_attribute_count,
    };
}

static cecs_buffer_attribute_builder_buffer_info cecs_mesh_builder_get_vertex_info(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    void *userdata
) {
    assert(userdata == NULL && "error: userdata not expected");
    (void)userdata;

    cecs_buffer_storage_attachment *storage = cecs_graphics_world_get_buffer_attachments(
        builder->graphics_world,
        attribute_id
    );
    CECS_UNION_IS_ASSERT(cecs_vertex_storage_attachment, cecs_stream_storage_attachment, storage->stream);
    cecs_vertex_storage_attachment *vertex_info = &CECS_UNION_GET_UNCHECKED(cecs_vertex_storage_attachment, storage->stream);
    return (cecs_buffer_attribute_builder_buffer_info){
        .attribute_count = &vertex_info->current_attribute_count,
        .attribute_stride = &vertex_info->attribute_stride,
        .max_attribute_count = &vertex_info->max_attribute_count,
    };
}

static cecs_buffer_attribute_builder_buffer_info cecs_mesh_builder_get_index_info(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    void *userdata
) {
    assert(userdata != NULL && "error: userdata not set to cecs_index_format_info");
    cecs_index_format_info *format_info = (cecs_index_format_info *)userdata;

    assert(attribute_id == format_info->id && "error: index format mismatch");
    cecs_buffer_storage_attachment *storage = cecs_graphics_world_get_buffer_attachments(
        builder->graphics_world,
        attribute_id
    );
    CECS_UNION_IS_ASSERT(cecs_index_storage_attachment, cecs_stream_storage_attachment, storage->stream);
    cecs_index_storage_attachment *index_info = &CECS_UNION_GET_UNCHECKED(cecs_index_storage_attachment, storage->stream);
    
    return (cecs_buffer_attribute_builder_buffer_info){
        .attribute_count = &index_info->current_index_count,
        .attribute_stride = &format_info->size,
        .max_attribute_count = &index_info->max_index_count,
    };
}

typedef struct cecs_mesh_builder_index_format_info {
    cecs_index_format_info format_info;
    WGPUIndexFormat index_format;
} cecs_mesh_builder_index_format_info;

static cecs_buffer_attribute_builder_buffer_info cecs_mesh_builder_index_info_init(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    const size_t attributes_count,
    const size_t attribute_size,
    void *userdata
) {
    assert(userdata != NULL && "error: userdata not set to cecs_index_format_info");
    cecs_mesh_builder_index_format_info *format_info = (cecs_mesh_builder_index_format_info *)userdata;
    
    assert(attribute_id == format_info->format_info.id && "error: index format mismatch");
    assert(attribute_size == format_info->format_info.size && "error: index size mismatch");
    (void)attribute_size;

    cecs_buffer_storage_attachment default_attachment = cecs_buffer_storage_attachment_create_index_uninitialized(
        (cecs_index_storage_attachment){
            .current_index_count = 0,
            .max_index_count = attributes_count,
            .index_format = format_info->index_format,
        }
    );
    cecs_index_storage_attachment *storage_index_info = cecs_graphics_world_get_or_set_index_buffer_attachments(
        builder->graphics_world,
        attribute_id,
        &default_attachment
    );
    assert(storage_index_info->index_format == format_info->index_format && "error: index format mismatch");

    return (cecs_buffer_attribute_builder_buffer_info){
        .attribute_count = &storage_index_info->current_index_count,
        .attribute_stride = &format_info->format_info.size,
        .max_attribute_count = &storage_index_info->max_index_count,
    };
}

cecs_mesh_builder cecs_mesh_builder_create(cecs_graphics_world *graphics_world, cecs_mesh_builder_descriptor descriptor, cecs_arena *builder_arena) {
    return (cecs_mesh_builder){
        .vertex_builder = cecs_attribute_builder_create(graphics_world, builder_arena, descriptor.vertex_attributes_expected_count),
        .index_builder = cecs_attribute_builder_create(graphics_world, builder_arena, 1),
        .descriptor = descriptor,
        .bounding_radius = 0.0f,
    };
}

cecs_mesh_builder cecs_mesh_builder_create_from(const cecs_mesh_builder *builder, cecs_mesh_builder_descriptor descriptor) {
    return cecs_mesh_builder_create(builder->vertex_builder.graphics_world, descriptor, builder->vertex_builder.builder_arena);
}

[[maybe_unused]]
static cecs_buffer_storage_attachment *cecs_mesh_builder_build_vertex_attribute(
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    const cecs_vertex_attribute_id id
) {
    return cecs_attribute_builder_build_attribute(
        &builder->vertex_builder,
        context,
        id,
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        cecs_mesh_builder_get_vertex_info,
        NULL
    );
}

static cecs_buffer_attribute_range cecs_mesh_builder_build_vertices(
    cecs_world *world,
    cecs_mesh_builder *builder,
    cecs_graphics_context *context
) {
    return cecs_attribute_builder_build_into(
        world,
        &builder->vertex_builder,
        context,
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        cecs_mesh_builder_get_vertex_info,
        NULL
    );
}

static cecs_buffer_storage_attachment *cecs_mesh_builder_build_indices(
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    const cecs_vertex_index_id id,
    const size_t index_format_size
) {
    return cecs_attribute_builder_build_attribute(
        &builder->index_builder,
        context,
        id,
        WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
        cecs_mesh_builder_get_index_info,
        &(cecs_index_format_info){id, index_format_size}
    );
}

cecs_mesh cecs_mesh_builder_build_into(
    cecs_world *world,
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    cecs_index_stream *out_index_stream
) {
    const cecs_buffer_attribute_range vertex_entities = cecs_mesh_builder_build_vertices(world, builder, context);

    bool has_index_stream = !cecs_attribute_builder_is_clear(&builder->index_builder);
    assert((out_index_stream != NULL) == has_index_stream && "error: index stream - out_component mismatch");
    if (has_index_stream) {
        cecs_index_format_info format_info = cecs_index_format_info_from(builder->descriptor.index_format);

        cecs_mesh_builder_build_indices(builder, context, format_info.id, format_info.size);
        *out_index_stream = (cecs_index_stream){
            .format = builder->descriptor.index_format,
            .index_count = cecs_exclusive_range_length(builder->index_builder.attribute_range),
            .first_index = builder->index_builder.attribute_range.start,
        };
    }

    return (cecs_mesh){
        .vertex_entities = vertex_entities.attribute_entities,
        .vertex_attribute_references = vertex_entities.attribute_references,
        .bounding_radius = builder->bounding_radius,
    };
}

bool cecs_mesh_builder_is_clear(const cecs_mesh_builder *builder) {
    return cecs_attribute_builder_is_clear(&builder->vertex_builder)
        && cecs_attribute_builder_is_clear(&builder->index_builder);
}

static cecs_mesh_builder *cecs_mesh_builder_clear_vertices(cecs_mesh_builder *builder) {
    cecs_attribute_builder_clear(&builder->vertex_builder, cecs_mesh_builder_get_vertex_info, NULL);
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_clear_vertex_attribute(cecs_mesh_builder *builder, cecs_vertex_attribute_id attribute_id) {
    cecs_attribute_builder_clear_attribute(&builder->vertex_builder, attribute_id, cecs_mesh_builder_get_vertex_info, NULL);
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_set_vertex_attribute(
    cecs_mesh_builder *builder,
    cecs_vertex_attribute_id attribute_id,
    void *attributes_data,
    const size_t attributes_count,
    const size_t attribute_size
) {
    cecs_attribute_builder_set_attribute(
        &builder->vertex_builder,
        attribute_id,
        attributes_data,
        attributes_count,
        attribute_size,
        cecs_mesh_builder_get_vertex_info_init,
        NULL
    );
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_clear_indices(cecs_mesh_builder *builder) {
    size_t index_format_size;
    cecs_attribute_builder_clear_attribute(
        &builder->vertex_builder,
        builder->descriptor.index_format,
        cecs_mesh_builder_get_index_info,
        &index_format_size
    );
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_set_indices(
    cecs_mesh_builder *builder,
    void *indices_data,
    size_t indices_count
) {
    cecs_mesh_builder_index_format_info format_info = {
        .format_info = cecs_index_format_info_from(builder->descriptor.index_format),
        .index_format = builder->descriptor.index_format,
    };
    cecs_attribute_builder_set_attribute(
        &builder->index_builder,
        format_info.format_info.id,
        indices_data,
        indices_count,
        format_info.format_info.size,
        cecs_mesh_builder_index_info_init,
        &format_info
    );
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_clear(cecs_mesh_builder *builder) {
    cecs_mesh_builder_clear_vertices(builder);
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

static cecs_buffer_attribute_builder_buffer_info cecs_instance_builder_get_instance_info(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    void *userdata
) {
    assert(userdata == NULL && "error: userdata not expected");
    (void)userdata;

    cecs_buffer_storage_attachment *storage = cecs_graphics_world_get_buffer_attachments(
        builder->graphics_world,
        attribute_id
    );
    CECS_UNION_IS_ASSERT(cecs_instance_storage_attachment, cecs_stream_storage_attachment, storage->stream);
    cecs_instance_storage_attachment *instance_info = &CECS_UNION_GET_UNCHECKED(cecs_instance_storage_attachment, storage->stream);
    return (cecs_buffer_attribute_builder_buffer_info){
        .attribute_count = &instance_info->current_attribute_count,
        .attribute_stride = &instance_info->attribute_stride,
        .max_attribute_count = &instance_info->max_attribute_count,
    };
}

static cecs_buffer_attribute_builder_buffer_info cecs_instance_builder_get_instance_info_init(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    const size_t attributes_count,
    const size_t attribute_size,
    void *userdata
) {
    assert(userdata == NULL && "error: userdata not expected");
    (void)userdata;

    cecs_buffer_storage_attachment default_attachment = cecs_buffer_storage_attachment_create_instance_uninitialized(
        (cecs_instance_storage_attachment){
            .max_attribute_count = attributes_count,
            .current_attribute_count = 0,
            .attribute_stride = attribute_size,
        }
    );
    cecs_instance_storage_attachment *storage_instance_info = cecs_graphics_world_get_or_set_instance_buffer_attachments(
        builder->graphics_world,
        attribute_id,
        &default_attachment
    );

    assert(storage_instance_info->attribute_stride == attribute_size && "error: attribute size mismatch");
    return (cecs_buffer_attribute_builder_buffer_info){
        .attribute_count = &storage_instance_info->current_attribute_count,
        .attribute_stride = &storage_instance_info->attribute_stride,
        .max_attribute_count = &storage_instance_info->max_attribute_count,
    };
}

cecs_instance_builder cecs_instance_builder_create(cecs_graphics_world *graphics_world, const cecs_instance_builder_descriptor descriptor, cecs_arena *builder_arena) {
    return (cecs_instance_builder){
        .instance_builder = cecs_attribute_builder_create(graphics_world, builder_arena, descriptor.instance_attributes_expected_count),
        .descriptor = descriptor,
    };
}

cecs_instance_group cecs_instance_builder_build_into(cecs_world *world, cecs_instance_builder *builder, cecs_graphics_context *context) {
    cecs_buffer_attribute_range range = cecs_attribute_builder_build_into(
        world,
        &builder->instance_builder,
        context,
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        cecs_instance_builder_get_instance_info,
        NULL
    );
    return (cecs_instance_group){
        .instance_entities = range.attribute_entities,
        .instance_attribute_references = range.attribute_references,
    };
}

cecs_instance_builder *cecs_instance_builder_clear_instance_attribute(cecs_instance_builder *builder, const cecs_instance_attribute_id attribute_id) {
    cecs_attribute_builder_clear_attribute(&builder->instance_builder, attribute_id, cecs_instance_builder_get_instance_info, NULL);
    return builder;
}

cecs_instance_builder *cecs_instance_builder_set_instance_attribute(
    cecs_instance_builder *builder,
    const cecs_instance_attribute_id attribute_id,
    void *attributes_data,
    const size_t attributes_count,
    const size_t attribute_size
) {
    cecs_attribute_builder_set_attribute(
        &builder->instance_builder,
        attribute_id,
        attributes_data,
        attributes_count,
        attribute_size,
        cecs_instance_builder_get_instance_info_init,
        NULL
    );
    return builder;
}

cecs_instance_builder *cecs_instance_builder_clear_attribute(cecs_instance_builder *builder, const cecs_instance_attribute_id attribute_id) {
    cecs_attribute_builder_clear_attribute(&builder->instance_builder, attribute_id, cecs_instance_builder_get_instance_info, NULL);
    return builder;
}

bool cecs_instance_builder_is_clear(const cecs_instance_builder *builder) {
    return cecs_attribute_builder_is_clear(&builder->instance_builder);
}

cecs_instance_builder *cecs_instance_builder_clear(cecs_instance_builder *builder) {
    cecs_attribute_builder_clear(&builder->instance_builder, cecs_instance_builder_get_instance_info, NULL);
    return builder;
}

cecs_instance_group cecs_instance_builder_build_into_and_clear(cecs_world *world, cecs_instance_builder *builder, cecs_graphics_context *context) {
    cecs_instance_group group = cecs_instance_builder_build_into(world, builder, context);
    cecs_instance_builder_clear(builder);
    return group;
}
