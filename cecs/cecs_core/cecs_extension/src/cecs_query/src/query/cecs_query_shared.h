#ifndef CECS_QUERY_SHARED_H
#define CECS_QUERY_SHARED_H

#include <world/cecs_world_components.h>
#include "descriptor/cecs_query_descriptor_shared.h"

void cecs_query_shared_acquire(
    const cecs_world_components *const components,
    const cecs_query_descriptor_shared *const descriptor,
    cecs_query_result *const out_result
);
void cecs_query_shared_acquire_mut(
    cecs_world_components *const components,
    const cecs_query_descriptor_shared *const descriptor,
    cecs_allocator *const allocators, // XXX: NULLABLE!
    cecs_query_result *const out_result
);

void cecs_query_shared_release(
    const cecs_world_components *const components,
    const cecs_query_descriptor_shared *const descriptor,
    cecs_query_result *const result
);
void cecs_query_shared_release_mut(
    cecs_world_components *const components,
    const cecs_query_descriptor_shared *const descriptor,
    cecs_query_result *const result
);

#endif
