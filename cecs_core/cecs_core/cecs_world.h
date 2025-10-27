#ifndef CECS_WORLD_H
#define CECS_WORLD_H

#include "world/cecs_entity_storage.h"
#include "world/cecs_registry.h" 

typedef struct cecs_world_components {
    cecs_dynarray component_storages;
} cecs_world_components;

typedef struct cecs_world {
    cecs_entity_storage entity_storage;
    cecs_world_components components;
} cecs_world;

#endif