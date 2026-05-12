#include "cecs_query.h"
#include "query/cecs_query_shared.h"
#include <cecs_error.h>

void cecs_query_acquire(
    const cecs_world_components *const components,
    const cecs_query_descriptor *const descriptor,
    cecs_query_result *const out_result
) {
    const cecs_query_descriptor_type type = (cecs_query_descriptor_type)descriptor->type;
    switch (type){
    case cecs_query_descriptor_type_shared:
        cecs_query_shared_acquire(components, &descriptor->descriptor.shared, out_result);
        break;
    case cecs_query_descriptor_type_exclusive:
        // cecs_query_exclusive_acquire(components, &descriptor->descriptor.exclusive, out_result);
        break;
    default:
        cecs_debugbreak_unreachable("invalid query descriptor type");
    }
}

void cecs_query_acquire_mut(
    cecs_world_components *const components,
    const cecs_query_descriptor *const descriptor,
    cecs_allocator *const allocators,
    cecs_query_result *const out_result
) {
    const cecs_query_descriptor_type type = (cecs_query_descriptor_type)descriptor->type;
    switch (type){
    case cecs_query_descriptor_type_shared:
        cecs_query_shared_acquire_mut(components, &descriptor->descriptor.shared, allocators, out_result);
        break;
    case cecs_query_descriptor_type_exclusive:
        // cecs_query_exclusive_acquire_mut(components, &descriptor->descriptor.exclusive, allocators, out_result);
        break;
    default:
        cecs_debugbreak_unreachable("invalid query descriptor type");
    }
}

void cecs_query_release(
    const cecs_world_components *const components,
    const cecs_query_descriptor *const descriptor,
    cecs_query_result *const result
) {
    const cecs_query_descriptor_type type = (cecs_query_descriptor_type)descriptor->type;
    switch (type){
    case cecs_query_descriptor_type_shared:
        cecs_query_shared_release(components, &descriptor->descriptor.shared, result);
        break;
    case cecs_query_descriptor_type_exclusive:
        // cecs_query_exclusive_release(components, &descriptor->descriptor.exclusive, result);
        break;
    default:
        cecs_debugbreak_unreachable("invalid query descriptor type");
    }
}
void cecs_query_release_mut(
    cecs_world_components *const components,
    const cecs_query_descriptor *const descriptor,
    cecs_query_result *const result
) {
    const cecs_query_descriptor_type type = (cecs_query_descriptor_type)descriptor->type;
    switch (type){
    case cecs_query_descriptor_type_shared:
        cecs_query_shared_release_mut(components, &descriptor->descriptor.shared, result);
        break;
    case cecs_query_descriptor_type_exclusive:
        // cecs_query_exclusive_release_mut(components, &descriptor->descriptor.exclusive, result);
        break;
    default:
        cecs_debugbreak_unreachable("invalid query descriptor type");
    }
}