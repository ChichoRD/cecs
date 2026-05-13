#include "cecs_world.h"

extern inline cecs_world cecs_world_create(void);
cecs_world cecs_world_create_with(
    cecs_allocator *const allocator,
    const size_t initial_entity_capacity,
    const size_t initial_component_types_capacity
) {
    return (cecs_world){
        .entities = cecs_entity_storage_create_with_capacity(allocator, initial_entity_capacity),
        .components = cecs_world_components_create_with_capacity(allocator, initial_component_types_capacity),
    };
}

void cecs_world_destroy(cecs_world *const world, cecs_allocator *const allocator) {
    cecs_world_components_destroy(&world->components, allocator);
    cecs_entity_storage_destroy(&world->entities, allocator);
}

extern inline cecs_entity cecs_world_alloc_entity(cecs_world *const world, cecs_allocator *const allocator);
extern inline void cecs_world_free_entity(cecs_world *const world, const cecs_entity entity);


cecs_component_type cecs_world_register_component(
    cecs_world *const world,
    cecs_allocator *const allocator,
    const cecs_component_storage_value storage_type,
    const size_t component_size,
    const size_t initial_capacity)
{
    const size_t registry_index = cecs_world_components_count(&world->components);
    cecs_debugbreak_fail_unless(
        registry_index <= CECS_COMPONENT_TYPE_ID_TYPE_MAX,
        "fatal error: exceeded maximum number of component types supported by cecs_world_register_component"
    );

    cecs_debugbreak_fail_unless(storage_type != cecs_component_storage_type_none, "fatal error: cecs_world_register_component was called with storage_type 'none'");
    cecs_component_storage storage;
    switch (storage_type) {
    case cecs_component_storage_type_none:
        cecs_unreachable();
        break;
    case cecs_component_storage_type_sparse_set:
        storage = cecs_component_storage_create_sparse_set(allocator, initial_capacity, component_size);
        break;
    default:
        cecs_unimplemented_fail(
            "unimplemented error: cecs_world_register_component was called with an unsupported storage type"
        );
    }
    cecs_registry *const registry = cecs_world_components_push_registry(&world->components, allocator);
    *registry = cecs_registry_create(
        cecs_component_registry_create(storage, /* FIXME: decide default group creation */ cecs_component_registry_groups_create(), component_size, NULL)
    );
    // TODO: standardize identifiers
    return (cecs_component_type){
        .id = (uint32_t)registry_index,
        .storage_type = storage_type,
    };
}

extern cecs_registry *cecs_world_components_get_registry_ptr(const cecs_world_components *const components, const size_t index);
cecs_view cecs_world_acquire_view(const cecs_world *const world, const cecs_component_type component) {
    const cecs_component_registry *unused;
    return cecs_view_create(
        cecs_world_components_acquire_registry(&world->components, component.id, &unused),
        component
    );
}
cecs_view_mut cecs_world_acquire_view_mut(cecs_world *const world, const cecs_component_type component) {
    cecs_component_registry *unused;
    return cecs_view_mut_create(
        cecs_world_components_acquire_registry_mut(&world->components, component.id, &unused),
        component
    );    
}
cecs_view_alloc cecs_world_acquire_view_alloc_take(cecs_world *const world, cecs_allocator *const allocator, const cecs_component_type component) {
    cecs_view_mut view = cecs_world_acquire_view_mut(world, component);
    return cecs_view_alloc_create_take(
        allocator,
        &view
    );
}
cecs_view_alloc cecs_world_acquire_view_alloc(cecs_world *const world, const cecs_allocator allocator, const cecs_component_type component) {
    return cecs_view_alloc_create(
        allocator,
        cecs_world_acquire_view_mut(world, component)
    );
}

void cecs_world_release_view(const cecs_world *const world, cecs_view *const view) {
    cecs_view_release(view, &world->components);
}

void cecs_world_release_view_mut(cecs_world *const world, cecs_view_mut *const view) {
    cecs_view_mut_release(view, &world->components);
}

void cecs_world_release_view_alloc(cecs_world *const world, cecs_view_alloc *const view) {
    cecs_unimplemented_fail(
        "TODO: if user acquired this view with a partition of our allocator we should try to merge the remaining free space back."
        "note: manually release the view with `cecs_view_alloc_release` for now, and then free the allocator if it was heap allocated"
    );
    (void)world;
    (void)view;
    // cecs_view_alloc_release(view, &world->components);
}
