#ifndef CECS_MESH_BUILDER_H
#define CECS_MESH_BUILDER_H

#include "component/cecs_mesh.h"
#include "context/cecs_graphics_context.h"
#include "cecs_graphics_world.h"

typedef struct cecs_mesh_builder_descriptor {
    cecs_entity_id mesh_id;
    size_t vertex_attributes_expected_count;
    WGPUIndexFormat index_format;
    bool remove_on_build;
} cecs_mesh_builder_descriptor;


typedef struct cecs_mesh_builder {
    cecs_graphics_world *graphics_world;
    cecs_mesh_builder_descriptor descriptor;
    cecs_arena *builder_arena;
    cecs_sparse_set vertex_attribute_ids;
    cecs_entity_id_range vertex_range;
    cecs_entity_id_range index_range;
    float bounding_radius;
} cecs_mesh_builder;

cecs_mesh_builder cecs_mesh_builder_create(cecs_graphics_world *graphics_world, cecs_mesh_builder_descriptor descriptor, cecs_arena *builder_arena);
cecs_mesh *cecs_mesh_builder_build_into(cecs_world *world, cecs_mesh_builder *builder, cecs_graphics_context *context);

cecs_mesh_builder *cecs_mesh_builder_clear_vertex_attribute(cecs_mesh_builder *builder, cecs_vertex_attribute_id attribute_id);

cecs_mesh_builder *cecs_mesh_builder_set_vertex_attribute(
    cecs_mesh_builder *builder,
    cecs_vertex_attribute_id attribute_id,
    void *attributes_data,
    size_t attributes_count,
    size_t attribute_size
);
// TODO: clears should just allow ovewrite
cecs_mesh_builder *cecs_mesh_builder_clear_indices(cecs_mesh_builder *builder);
cecs_mesh_builder *cecs_mesh_builder_set_indices(
    cecs_mesh_builder *builder,
    void *indices_data,
    size_t indices_count
);

cecs_mesh_builder *cecs_mesh_builder_clear(cecs_mesh_builder *builder);
cecs_mesh *cecs_mesh_builder_build_into_and_clear(cecs_world *world, cecs_mesh_builder *builder, cecs_graphics_context *context);

#endif