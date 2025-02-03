#ifndef CECS_MESH_H
#define CECS_MESH_H

#include <stddef.h>
#include <cecs_core/cecs_core.h>
#include "cecs_vertex.h"

/// @brief @ref vertex_entities is the range of entities in the graphics world that contain the vertex data for this mesh.
/// @ref vertex_attribute_references is the range of entities in the mesh world that contain the index of each attribute in the graphics world.
/// the same entity that has a mesh migh have a cecs_index_stream component that contains the index data for the mesh.
typedef struct cecs_mesh {
    cecs_entity_id_range vertex_entities;
    cecs_entity_id_range vertex_attribute_references;
    float bounding_radius; 
} cecs_mesh;
CECS_COMPONENT_DECLARE(cecs_mesh);

static inline uint32_t cecs_mesh_vertex_count(cecs_mesh mesh) {
    return cecs_exclusive_range_length(mesh.vertex_entities);
}

static inline cecs_vertex_stream cecs_mesh_get_vertex_stream(cecs_mesh mesh, size_t stride) {
    return (cecs_vertex_stream){
        .offset = mesh.vertex_entities.start * stride,
        .size = cecs_exclusive_range_length(mesh.vertex_entities) * stride
    };
}

static inline cecs_raw_stream cecs_mesh_get_raw_vertex_stream(cecs_mesh mesh, size_t stride, const cecs_vertex_buffer *vertex_buffer) {
    return cecs_raw_stream_from_vertex(cecs_mesh_get_vertex_stream(mesh, stride), vertex_buffer);
}

typedef struct cecs_instance_group {
    cecs_entity_id_range instance_entities;
    cecs_entity_id_range instance_attribute_references;
} cecs_instance_group;
CECS_COMPONENT_DECLARE(cecs_instance_group);

static inline uint32_t cecs_instance_group_instance_count(cecs_instance_group group) {
    return cecs_exclusive_range_length(group.instance_entities);
}

static inline cecs_instance_stream cecs_instance_group_get_instance_stream(cecs_instance_group group, size_t stride) {
    return (cecs_instance_stream){
        .offset = group.instance_entities.start * stride,
        .size = cecs_exclusive_range_length(group.instance_entities) * stride
    };
}

static inline cecs_raw_stream cecs_instance_group_get_raw_instance_stream(cecs_instance_group group, size_t stride, const cecs_instance_buffer *instance_buffer) {
    return cecs_raw_stream_from_instance(cecs_instance_group_get_instance_stream(group, stride), instance_buffer);
}

#endif