#include "cecs_world.h"

extern inline cecs_world_components cecs_world_components_create(void);
extern inline cecs_world_components cecs_world_components_create_with_capacity(cecs_allocator *const allocator, const size_t component_types_capacity);

inline size_t cecs_world_components_count(const cecs_world_components *const components);
extern inline cecs_registry *cecs_world_components_push_registry(cecs_world_components *const components, cecs_allocator *const allocator);

extern inline cecs_registry *cecs_world_components_get_registry_ptr(const cecs_world_components *const components, const size_t index) {
    extern void *cecs_dynarray_get_ptr(const cecs_dynarray *arr, const size_t index, const size_t size);
    return (cecs_registry *)cecs_dynarray_get_ptr(&components->registries, index, sizeof(cecs_registry));
}
extern inline const cecs_registry *cecs_world_components_get_registry(const cecs_world_components *const components, const size_t index);
extern inline cecs_registry *cecs_world_components_get_registry_mut(cecs_world_components *const components, const size_t index);


extern inline cecs_entity cecs_world_alloc_entity(cecs_world *const world, cecs_allocator *const allocator);
extern inline void cecs_world_free_entity(cecs_world *const world, const cecs_entity entity);


cecs_component_type cecs_world_register_component(
    cecs_world *const world,
    cecs_allocator *const allocator,
    const cecs_component_storage_value storage_type
) {
    const size_t registry_index = cecs_world_components_count(&world->components);
    cecs_assert_or_exit(
        registry_index <= UINT32_MAX, // TODO: replace with CECS_COMPONENT_ID_MAX when defined
        "fatal error: exceeded maximum number of component types supported by cecs_world_register_component"
    );

    cecs_registry *const registry = cecs_world_components_push_registry(&world->components, allocator);
    *registry = cecs_registry_create((cecs_component_registry){0});
    cecs_unimplemented_fail(
        "unimplemented error: cecs_world_register_component is not yet implemented"
    );
    return (cecs_component_type){
        .id = (uint32_t)registry_index,
        .storage_type = storage_type,
    };
}
void *cecs_world_insert_component(
    cecs_world *const world,
    cecs_allocator *const allocator,
    const cecs_entity entity,
    const cecs_component_type component_type,
    const size_t component_size
) {
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(&world->entity_storage, entity);
    cecs_registry *const registry = cecs_world_components_get_registry_mut(&world->components, component_type.id);
    // TODO: think whether user acquires the storage for a period of time, with an "inserter" struct, or if we just provide direct access each time
    // cecs_component_registry *const component_registry = cecs_registry_acquire_mut_or_exit();
    cecs_unimplemented_fail(
        "unimplemented error: cecs_world_insert_component is not yet implemented"
    );
    return NULL;
}

