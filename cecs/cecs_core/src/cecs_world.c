#include "cecs_world.h"


extern inline cecs_entity cecs_world_alloc_entity(cecs_world *const world, cecs_allocator *const allocator);
extern inline void cecs_world_free_entity(cecs_world *const world, const cecs_entity entity);


cecs_component_type cecs_world_register_component(
    cecs_world *const world,
    cecs_allocator *const allocator,
    const cecs_component_storage_value storage_type,
    const size_t component_size
) {
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
        // HACK: default to initial capacity of 16 components
        storage = cecs_component_storage_create_sparse_set(allocator, 16ull, component_size);
        break;
    default:
        cecs_unimplemented_fail(
            "unimplemented error: cecs_world_register_component was called with an unsupported storage type"
        );
    }
    cecs_registry *const registry = cecs_world_components_push_registry(&world->components, allocator);
    cecs_unimplemented_fail(
        "unimplemented error: signature of 'cecs_component_registry_create' changed to support groups but we still cannot provide groups at the time of registry creation, cecs_world_register_component needs to be updated to handle this change"
    );
    (void)storage;
    (void)registry;
    // *registry = cecs_registry_create(cecs_component_registry_create(storage, component_size, NULL));
    // return (cecs_component_type){
    //     .id = (uint32_t)registry_index,
    //     .storage_type = storage_type,
    // };
}

extern cecs_registry *cecs_world_components_get_registry_ptr(const cecs_world_components *const components, const size_t index);
cecs_view cecs_world_acquire_view(const cecs_world *const world, const cecs_component_type component) {
    cecs_registry *const registry = cecs_world_components_get_registry_ptr(&world->components, component.id);
    const cecs_component_registry *unused;
    return cecs_view_create(
        cecs_registry_acquire_or_exit(registry, &unused),
        component
    );
}
cecs_view_mut cecs_world_acquire_view_mut(cecs_world *const world, const cecs_component_type component) {
    cecs_registry *const registry = cecs_world_components_get_registry_mut(&world->components, component.id);
    cecs_component_registry *unused;
    return cecs_view_mut_create(
        cecs_registry_acquire_mut_or_exit(registry, &unused),
        component
    );    
}
cecs_view_alloc cecs_world_acquire_view_alloc_from(cecs_world *const world, cecs_allocator *const allocator, const cecs_component_type component) {
    cecs_view_mut view = cecs_world_acquire_view_mut(world, component);
    return cecs_view_alloc_create(
        allocator,
        &view
    );
}

void cecs_world_release_view(cecs_world *const world, cecs_view *const view) {
    cecs_view_release(view, &world->components);
}

void cecs_world_release_view_mut(cecs_world *const world, cecs_view_mut *const view) {
    cecs_view_mut_release(view, &world->components);
}

void cecs_world_release_view_alloc(cecs_world *const world, cecs_view_alloc *const view) {
    cecs_unimplemented_fail("TODO: if user acquired this view with a partition of our allocator we should try to merge the remaining free space back");
    (void)world;
    (void)view;
    // cecs_view_alloc_release(view, &world->components);
}
