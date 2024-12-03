#ifndef CECS_MESH_H
#define CECS_MESH_H

#include <stddef.h>
#include <cecs_core/cecs_core.h>
#include "cecs_vertex.h"

typedef struct cecs_mesh {
    size_t vertex_count;
    cecs_entity_id_range vertex_attribute_references;
    float bounding_radius; 
} cecs_mesh;
CECS_COMPONENT_IMPLEMENT(cecs_mesh);

#endif