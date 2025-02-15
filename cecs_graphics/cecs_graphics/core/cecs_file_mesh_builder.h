#ifndef CECS_FILE_MESH_BUILDER_H
#define CECS_FILE_MESH_BUILDER_H

#include "builder/cecs_cgltf.h"
#include "component/cecs_vertex.h"
#include "component/cecs_mesh.h"
#include "cecs_mesh_builder.h"

typedef enum cecs_attribute_copy {
    cecs_attribute_copy_expect_exact = 0,

    cecs_attribute_copy_expect_larger_copy_padded,
    cecs_attribute_copy_expect_larger_copy_start,

    cecs_attribute_copy_expect_smaller_zero_fill_padded,
    cecs_attribute_copy_expect_smaller_zero_fill,
} cecs_attribute_copy;
typedef uint8_t cecs_attribute_copy_options;

// TODO: combine meshes together
// TODO: helper methods for multimesh
typedef struct cecs_file_mesh_builder_gltf {
    cecs_mesh_builder *mesh_builders;
    cgltf_data *data;
} cecs_file_mesh_builder_gltf;

cecs_file_mesh_builder_gltf cecs_file_mesh_builder_gltf_create(
    cecs_graphics_world *graphics_world,
    const cecs_mesh_builder_descriptor descriptor,
    cecs_arena *builder_arena,
    const char *path
);
cecs_file_mesh_builder_gltf *cecs_file_mesh_builder_gltf_clear_builders(cecs_file_mesh_builder_gltf *builder);
cecs_file_mesh_builder_gltf *cecs_file_mesh_builder_gltf_clear_builders_and_data(cecs_file_mesh_builder_gltf *builder);

inline size_t cecs_file_mesh_builder_gltf_mesh_count(const cecs_file_mesh_builder_gltf *builder) {
    return builder->data->meshes_count;
}
inline cecs_arena *cecs_file_mesh_builder_gltf_arena(const cecs_file_mesh_builder_gltf *builder) {
    assert(builder->mesh_builders != NULL && "error: mesh builders not initialized");
    return builder->mesh_builders->vertex_builder.builder_arena;
}

bool cecs_file_mesh_builder_gltf_set_vertex_attribute(
    cecs_file_mesh_builder_gltf *builder,
    const cgltf_attribute_type attribute_type,
    const cecs_vertex_attribute_id attribute_id,
    const size_t attribute_size,
    const size_t builder_index,
    const cecs_attribute_copy_options copy_options
);
size_t cecs_file_mesh_builder_gltf_set_all_vertex_attributes(
    cecs_file_mesh_builder_gltf *builder,
    const cgltf_attribute_type attribute_type,
    const cecs_vertex_index_id attribute_id,
    const size_t attribute_size,
    const cecs_attribute_copy_options copy_options
);
bool cecs_file_mesh_builder_gltf_clear_vertex_attribute(
    cecs_file_mesh_builder_gltf *builder,
    const cecs_vertex_attribute_id attribute_id,
    const size_t builder_index
);

bool cecs_file_mesh_builder_gltf_set_indices(
    cecs_file_mesh_builder_gltf *builder,
    const size_t builder_index,
    const cecs_attribute_copy_options copy_options
);
size_t cecs_file_mesh_builder_gltf_set_all_indices(
    cecs_file_mesh_builder_gltf *builder,
    const cecs_attribute_copy_options copy_options
);
bool cecs_file_mesh_builder_gltf_clear_indices(
    cecs_file_mesh_builder_gltf *builder,
    const size_t builder_index
);

#endif