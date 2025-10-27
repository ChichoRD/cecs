#ifndef CECS_COMPONENT_REGISTRY_H
#define CECS_COMPONENT_REGISTRY_H

#include "registry/cecs_component_storage.h"

typedef struct cecs_component_registry {
    cecs_component_storage storage;
} cecs_component_registry;

#endif
