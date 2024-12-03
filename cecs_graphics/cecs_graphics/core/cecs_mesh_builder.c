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
        if (cecs_sparse_set_count_of_size(&builder->vertex_attribute_ids, sizeof(cecs_component_id)) == 0) {
            cecs_world_remove_entity_range(builder->graphics_world, builder->vertex_range);
            builder->vertex_range = (cecs_entity_id_range){0, 0};
        } else {
            // TODO: support for component arrays (range)
            for (cecs_entity_id e = builder->vertex_range.start; e < builder->vertex_range.end; ++e) {
                cecs_world_remove_component(
                    builder->graphics_world,
                    e,
                    attribute_id,
                    builder->graphics_world->components.discard.handle
                );
            }
        }
    } else {
        assert(false && "error: vertex attribute not found");
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

    // TODO: support for component arrays (range)
    for (size_t i = 0; i < attributes_count; i++) {
        cecs_entity_id id = (cecs_entity_id)(builder->vertex_range.start + i);
        cecs_world_set_component(
            builder->graphics_world,
            id,
            attribute_id,
            (uint8_t *)attributes_data + (i * attribute_size),
            attribute_size
        );
    }
    return builder;
}

cecs_mesh_builder *cecs_mesh_builder_clear_indices(cecs_mesh_builder *builder) {
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

    // TODO: support for component arrays (range)
    switch (index_format) {
    case WGPUIndexFormat_Uint16: {
        for (size_t i = 0; i < indices_count; i++) {
            cecs_entity_id e = builder->index_range.start + i;
            cecs_world_set_component(
                builder->graphics_world,
                e,
                CECS_COMPONENT_ID(cecs_vertex_index_u16),
                (cecs_vertex_index_u16 *)indices_data + i,
                sizeof(cecs_vertex_index_u16)
            );
        }
        break;
    }
    case WGPUIndexFormat_Uint32: {
        for (size_t i = 0; i < indices_count; i++) {
            cecs_entity_id e = builder->index_range.start + i;
            cecs_world_set_component(
                builder->graphics_world,
                e,
                CECS_COMPONENT_ID(cecs_vertex_index_u32),
                (cecs_vertex_index_u32 *)indices_data + i,
                sizeof(cecs_vertex_index_u32)
            );
        }
        break;
    }
    default: {
        assert(false && "unreachable: invalid index format");
        exit(EXIT_FAILURE);
    }
    }
    return builder;
}
