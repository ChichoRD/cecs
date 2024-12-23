#include <memory.h>
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

cecs_mesh *cecs_mesh_builder_build_into(cecs_world *world, cecs_mesh_builder *builder, cecs_graphics_context *context) {
    // TODO
    return NULL;
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
        cecs_stream_storage_attachment *storage = cecs_world_get_component_storage_attachments(
            builder->graphics_world,
            attribute_id
        );
        CECS_UNION_IS_ASSERT(cecs_vertex_attribute_storage_attachment, cecs_stream_storage_attachment, *storage);
        size_t *stored_vertex_count = &CECS_UNION_GET_UNCHECKED(cecs_vertex_attribute_storage_attachment, *storage).current_vertex_count;
        
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
        &(cecs_stream_storage_attachment)CECS_UNION_CREATE(
            cecs_vertex_attribute_storage_attachment,
            cecs_stream_storage_attachment,
            ((cecs_vertex_attribute_storage_attachment){
                .current_vertex_count = 0,
                .max_vertex_count = attributes_count,
                .attribute_size = attribute_size,
            })
        ),
        sizeof(cecs_stream_storage_attachment)
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
    cecs_vertex_index_id index_id = ~0;
    if (CECS_WORLD_HAS_COMPONENT_STORAGE_ATTACHMENTS(cecs_vertex_index_u16, builder->graphics_world)) {
        index_id = CECS_COMPONENT_ID(cecs_vertex_index_u16);
    } else if (CECS_WORLD_HAS_COMPONENT_STORAGE_ATTACHMENTS(cecs_vertex_index_u32, builder->graphics_world)) {
        index_id = CECS_COMPONENT_ID(cecs_vertex_index_u32);
    }

    if (index_id != ~0) {
        cecs_stream_storage_attachment *storage_index_info = cecs_world_get_component_storage_attachments(
            builder->graphics_world,
            index_id
        );
        CECS_UNION_IS_ASSERT(cecs_index_storage_attachment, cecs_stream_storage_attachment, *storage_index_info);
        size_t *stored_index_count = &CECS_UNION_GET_UNCHECKED(cecs_index_storage_attachment, *storage_index_info).current_index_count;
        assert(*stored_index_count >= index_count && "error: index count mismatch");

        *stored_index_count -= index_count;
    }

    cecs_world_remove_entity_range(builder->graphics_world, builder->index_range);
    builder->index_range = (cecs_entity_id_range){0, 0};
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_set_indices(
    cecs_mesh_builder *builder,
    void *indices_data,
    size_t indices_count,
    WGPUIndexFormat index_format
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
    switch (index_format) {
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
        &(cecs_stream_storage_attachment)CECS_UNION_CREATE(
            cecs_index_storage_attachment,
            cecs_stream_storage_attachment,
            ((cecs_index_storage_attachment){
                .current_index_count = 0,
                .max_index_count = indices_count,
                .index_format = index_format,
            })
        ),
        sizeof(cecs_stream_storage_attachment)
    );
    assert(storage_index_info->index_format == index_format && "error: index format mismatch");
    storage_index_info->current_index_count += indices_count;
    storage_index_info->max_index_count = max(storage_index_info->max_index_count, storage_index_info->current_index_count);

    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_clear(cecs_mesh_builder *builder) {
    size_t atribute_count = cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_component_id));
    cecs_component_id *remove_ids = calloc(
        cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_component_id)),
        atribute_count
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
    cecs_mesh_builder_clear_indices(builder);
    return builder;
}

cecs_mesh *cecs_mesh_builder_build_into_and_clear(cecs_world *world, cecs_mesh_builder *builder, cecs_graphics_context *context) {
    cecs_mesh *mesh = cecs_mesh_builder_build_into(world, builder, context);
    cecs_mesh_builder_clear(builder);
    return mesh;
}
