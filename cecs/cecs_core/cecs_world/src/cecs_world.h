#ifndef CECS_WORLD_H
#define CECS_WORLD_H

#include "world/cecs_entity_storage.h"
#include "world/cecs_world_components.h"
#include "world/cecs_view.h"

typedef struct cecs_world {
    cecs_entity_storage entities;
    cecs_world_components components;
} cecs_world;


inline cecs_world cecs_world_create(void) {
    return (cecs_world){
        .entities = cecs_entity_storage_create(),
        .components = cecs_world_components_create(),
    };
}
// XXX: subject to change as world acquires more functionality, eg. systems, resources, etc.
cecs_world cecs_world_create_with(
    cecs_allocator *const allocator,
    const size_t initial_entity_capacity,
    const size_t initial_component_types_capacity
);
void cecs_world_destroy(cecs_world *const world, cecs_allocator *const allocator);


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
    const size_t component_size,
    const size_t initial_capacity
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
// FIXME: name *_unchecked or *_implicit to signify that the caller must ensure the returned view is the sole owner
// of the allocator and the view's mutex guard. No other reference must alias these resources!
cecs_view_alloc cecs_world_acquire_view_alloc(cecs_world *const world, const cecs_allocator allocator, const cecs_component_type component);


void cecs_world_release_view(
    const cecs_world *const world,
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
