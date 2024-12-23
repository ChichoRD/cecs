#ifndef CECS_MESH_H
#define CECS_MESH_H

#include <stddef.h>
#include <cecs_core/cecs_core.h>
#include "cecs_vertex.h"

/// @brief @ref vertex_graphics_entities is the range of entities in the graphics world that contain the vertex data for this mesh.
/// @ref vertex_attribute_references is the range of entities in the mesh world that contain the index of each attribute in the graphics world.
/// the same entity that has a mesh migh have a cecs_index_stream component that contains the index data for the mesh.
typedef struct cecs_mesh {
    cecs_entity_id_range vertex_graphics_entities;
    cecs_entity_id_range vertex_attribute_references;
    float bounding_radius; 
} cecs_mesh;
CECS_COMPONENT_IMPLEMENT(cecs_mesh);

#endif