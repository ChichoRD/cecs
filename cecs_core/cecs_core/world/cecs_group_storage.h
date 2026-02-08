#ifndef CECS_GROUP_STORAGE_H
#define CECS_GROUP_STORAGE_H

#include "cecs_group.h"
#include "registry/cecs_component.h"
#include <cecs_core/container/cecs_dynarray.h>
#include <cecs_core/container/cecs_flatset.h>

typedef struct cecs_group_descriptor {
    cecs_flatset component_types;
    cecs_group group;
} cecs_group_descriptor;

typedef struct cecs_group_storage {
    cecs_dynarray descriptors;
} cecs_group_storage;

cecs_group_type cecs_group_storage_register_group(
    cecs_group_storage *const storage,
    cecs_allocator *const allocator,
    const cecs_group_storage_type storage_type,
    const cecs_component_type *const group_components,
    const size_t group_components_count
);

#endif
