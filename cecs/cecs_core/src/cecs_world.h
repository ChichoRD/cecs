#ifndef CECS_WORLD_H
#define CECS_WORLD_H

#include "world/cecs_entity_storage.h"
#include "world/cecs_world_components.h"
#include "world/cecs_view.h"

typedef struct cecs_world {
    cecs_entity_storage entities;
    cecs_world_components components;
} cecs_world;


inline cecs_entity cecs_world_alloc_entity(cecs_world *const world, cecs_allocator *const allocator) {
    return cecs_entity_storage_alloc_entity(&world->entities, allocator);
}
inline void cecs_world_free_entity(cecs_world *const world, const cecs_entity entity) {
    cecs_entity_storage_free_entity(&world->entities, entity);
}

// TODO: more defaults
cecs_component_type cecs_world_register_component(
    cecs_world *const world,
    cecs_allocator *const allocator,
    const cecs_component_storage_value storage_type,
    const size_t component_size
);

cecs_view cecs_world_acquire_view(
    const cecs_world *const world,
    const cecs_component_type component
);
cecs_view_mut cecs_world_acquire_view_mut(
    cecs_world *const world,
    const cecs_component_type component
);
// TODO: parameters maybe, user alloc? partitioned alloc?
cecs_view_alloc cecs_world_acquire_view_alloc_take(
    cecs_world *const world,
    cecs_allocator *const allocator,
    const cecs_component_type component
);
cecs_view_alloc cecs_world_acquire_view_alloc(cecs_world *const world, const cecs_allocator allocator, const cecs_component_type component);


void cecs_world_release_view(
    cecs_world *const world,
    cecs_view *const view
);
void cecs_world_release_view_mut(
    cecs_world *const world,
    cecs_view_mut *const view
);
void cecs_world_release_view_alloc(
    cecs_world *const world,
    cecs_view_alloc *const view
);

#endif
