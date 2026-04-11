#ifndef CECS_WORLD_COMPONENTS_H
#define CECS_WORLD_COMPONENTS_H

#include "cecs_registry.h" 

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
void cecs_world_components_destroy(cecs_world_components *const components, cecs_allocator *const allocator);

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


#endif
