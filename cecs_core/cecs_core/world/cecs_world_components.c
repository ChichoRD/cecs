#include "cecs_world_components.h"

extern inline cecs_world_components cecs_world_components_create(void);
extern inline cecs_world_components cecs_world_components_create_with_capacity(cecs_allocator *const allocator, const size_t component_types_capacity);
void cecs_world_components_destroy(cecs_world_components *const components, cecs_allocator *const allocator) {
    const size_t count = cecs_world_components_count(components);
    for (size_t i = 0; i < count; ++i) {
        cecs_registry *const registry = cecs_world_components_get_registry_mut(components, i);
        const size_t component_size = cecs_registry_get_unchecked(registry)->component_size;
        cecs_registry_destroy(registry, allocator, component_size);
    }
    cecs_dynarray_destroy(&components->registries, allocator, sizeof(cecs_registry));
}

inline size_t cecs_world_components_count(const cecs_world_components *const components);
extern inline cecs_registry *cecs_world_components_push_registry(cecs_world_components *const components, cecs_allocator *const allocator);

extern inline cecs_registry *cecs_world_components_get_registry_ptr(const cecs_world_components *const components, const size_t index) {
    extern void *cecs_dynarray_get_ptr(const cecs_dynarray *arr, const size_t index, const size_t size);
    return (cecs_registry *)cecs_dynarray_get_ptr(&components->registries, index, sizeof(cecs_registry));
}
extern inline const cecs_registry *cecs_world_components_get_registry(const cecs_world_components *const components, const size_t index);
extern inline cecs_registry *cecs_world_components_get_registry_mut(cecs_world_components *const components, const size_t index);
