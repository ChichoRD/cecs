#ifndef CECS_MESH_BUILDER_H
#define CECS_MESH_BUILDER_H

#include "component/cecs_mesh.h"
#include "context/cecs_graphics_context.h"
#include "cecs_graphics_world.h"

typedef struct cecs_buffer_attribute_builder {
    cecs_sparse_set attribute_ids;
    cecs_entity_id_range attribute_range;
    cecs_graphics_world *graphics_world;
    cecs_arena *builder_arena;
} cecs_buffer_attribute_builder;
typedef struct cecs_buffer_attribute_range {
    cecs_entity_id_range attribute_entities;
    cecs_entity_id_range attribute_references;
} cecs_buffer_attribute_range;

typedef struct cecs_buffer_attribute_builder_buffer_info {
    size_t *attribute_count;
    size_t *attribute_stride;
    size_t *max_attribute_count;
} cecs_buffer_attribute_builder_buffer_info;
typedef cecs_buffer_attribute_builder_buffer_info cecs_attribute_builder_get_buffer_info(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    void *userdata
);
typedef cecs_buffer_attribute_builder_buffer_info cecs_attribute_builder_get_buffer_info_init(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    const size_t attributes_count,
    const size_t attribute_size,
    void *userdata
);


cecs_buffer_attribute_builder cecs_attribute_builder_create(cecs_graphics_world *graphics_world, cecs_arena *builder_arena, const size_t expected_attribute_count);
cecs_buffer_attribute_range cecs_attribute_builder_build_into(
    cecs_world *world,
    cecs_buffer_attribute_builder *builder,
    cecs_graphics_context *context,
    const WGPUBufferUsageFlags usage,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
);

cecs_buffer_attribute_builder *cecs_attribute_builder_clear_attribute(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
);
cecs_buffer_attribute_builder *cecs_attribute_builder_set_attribute(
    cecs_buffer_attribute_builder *builder,
    const cecs_buffer_attribute_id attribute_id,
    void *attributes_data,
    const size_t attributes_count,
    const size_t attribute_size,
    cecs_attribute_builder_get_buffer_info_init *const get_buffer_info_init,
    void *userdata
);

bool cecs_attribute_builder_is_clear(const cecs_buffer_attribute_builder *builder);
cecs_buffer_attribute_builder *cecs_attribute_builder_clear(
    cecs_buffer_attribute_builder *builder,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
);
cecs_buffer_attribute_range cecs_attribute_builder_build_into_and_clear(
    cecs_world *world,
    cecs_buffer_attribute_builder *builder,
    cecs_graphics_context *context,
    const WGPUBufferUsageFlags usage,
    cecs_attribute_builder_get_buffer_info *const get_buffer_info,
    void *userdata
);


typedef struct cecs_mesh_builder_descriptor {
    size_t vertex_attributes_expected_count;
    WGPUIndexFormat index_format;
    bool remove_on_build;
} cecs_mesh_builder_descriptor;

typedef struct cecs_mesh_builder {
    cecs_buffer_attribute_builder vertex_builder;
    cecs_buffer_attribute_builder index_builder;
    cecs_mesh_builder_descriptor descriptor;
    float bounding_radius;
} cecs_mesh_builder;


cecs_mesh_builder cecs_mesh_builder_create(cecs_graphics_world *graphics_world, cecs_mesh_builder_descriptor descriptor, cecs_arena *builder_arena);
cecs_mesh_builder cecs_mesh_builder_create_from(const cecs_mesh_builder *builder, cecs_mesh_builder_descriptor descriptor);

cecs_mesh cecs_mesh_builder_build_into(
    cecs_world *world,
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    cecs_index_stream *out_index_stream
);

bool cecs_mesh_builder_is_clear(const cecs_mesh_builder *builder);
cecs_mesh_builder *cecs_mesh_builder_clear_vertex_attribute(cecs_mesh_builder *builder, cecs_vertex_attribute_id attribute_id);

cecs_mesh_builder *cecs_mesh_builder_set_vertex_attribute(
    cecs_mesh_builder *builder,
    cecs_vertex_attribute_id attribute_id,
    void *attributes_data,
    size_t attributes_count,
    size_t attribute_size
);

cecs_mesh_builder *cecs_mesh_builder_clear_indices(cecs_mesh_builder *builder);
cecs_mesh_builder *cecs_mesh_builder_set_indices(
    cecs_mesh_builder *builder,
    void *indices_data,
    size_t indices_count
);

cecs_mesh_builder *cecs_mesh_builder_clear(cecs_mesh_builder *builder);
cecs_mesh cecs_mesh_builder_build_into_and_clear(
    cecs_world *world,
    cecs_mesh_builder *builder,
    cecs_graphics_context *context,
    cecs_index_stream *out_index_stream
);

typedef struct cecs_instance_builder_descriptor  {
    size_t instance_attributes_expected_count;
} cecs_instance_builder_descriptor;

typedef struct cecs_instance_builder {
    cecs_buffer_attribute_builder instance_builder;
    cecs_instance_builder_descriptor descriptor;
} cecs_instance_builder;

cecs_instance_builder cecs_instance_builder_create(cecs_graphics_world *graphics_world, const cecs_instance_builder_descriptor descriptor, cecs_arena *builder_arena);
cecs_instance_group cecs_instance_builder_build_into(
    cecs_world *world,
    cecs_instance_builder *builder,
    cecs_graphics_context *context
);

cecs_instance_builder *cecs_instance_builder_clear_instance_attribute(cecs_instance_builder *builder, const cecs_instance_attribute_id attribute_id);
cecs_instance_builder *cecs_instance_builder_set_instance_attribute(
    cecs_instance_builder *builder,
    const cecs_instance_attribute_id attribute_id,
    void *attributes_data,
    const size_t attributes_count,
    const size_t attribute_size
);

cecs_instance_builder *cecs_instance_builder_clear_attribute(cecs_instance_builder *builder, const cecs_instance_attribute_id attribute_id);
bool cecs_instance_builder_is_clear(const cecs_instance_builder *builder);

cecs_instance_builder *cecs_instance_builder_clear(cecs_instance_builder *builder);
cecs_instance_group cecs_instance_builder_build_into_and_clear(
    cecs_world *world,
    cecs_instance_builder *builder,
    cecs_graphics_context *context
);

#endif