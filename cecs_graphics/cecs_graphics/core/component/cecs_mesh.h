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

#endif