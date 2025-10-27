#ifndef CECS_WORLD_H
#define CECS_WORLD_H

#include "world/cecs_entity_storage.h"
#include "world/cecs_registry.h" 

typedef struct cecs_world_components {
    cecs_dynarray registries;
} cecs_world_components;

inline cecs_world_components cecs_world_components_create(void) {
    return (cecs_world_components){
        .registries = cecs_dynarray_create(),
    };
}
inline cecs_world_components cecs_world_components_create_with_capacity(cecs_allocator *const allocator, const size_t component_types_capacity) {
    return (cecs_world_components){
        .registries = cecs_dynarray_create_with_capacity(allocator, component_types_capacity, sizeof(cecs_registry)),
    };
}
inline size_t cecs_world_components_count(const cecs_world_components *const components) {
    return cecs_dynarray_count(&components->registries);
}
inline cecs_registry *cecs_world_components_push_registry(cecs_world_components *const components, cecs_allocator *const allocator) {
    return (cecs_registry *)cecs_dynarray_push(&components->registries, allocator, sizeof(cecs_registry));
}
inline const cecs_registry *cecs_world_components_get_registry(const cecs_world_components *const components, const size_t index) {
    return (const cecs_registry *)cecs_dynarray_get(&components->registries, index, sizeof(cecs_registry));
}
inline cecs_registry *cecs_world_components_get_registry_mut(cecs_world_components *const components, const size_t index) {
    return (cecs_registry *)cecs_dynarray_get(&components->registries, index, sizeof(cecs_registry));
}

typedef struct cecs_world {
    cecs_entity_storage entity_storage;
    cecs_world_components components;
} cecs_world;


inline cecs_entity cecs_world_alloc_entity(cecs_world *const world, cecs_allocator *const allocator) {
    return cecs_entity_storage_alloc_entity(&world->entity_storage, allocator);
}
inline void cecs_world_free_entity(cecs_world *const world, const cecs_entity entity) {
    cecs_entity_storage_free_entity(&world->entity_storage, entity);
}

// FIXME: unimplemented
cecs_component_type cecs_world_register_component(
    cecs_world *const world,
    cecs_allocator *const allocator,
    const cecs_component_storage_value storage_type
);

// FIXME: unimplemented
void *cecs_world_insert_component(
    cecs_world *const world,
    cecs_allocator *const allocator,
    const cecs_entity entity,
    const cecs_component_type component_type,
    const size_t component_size
);

#endif