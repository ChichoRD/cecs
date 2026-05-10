#ifndef CECS_QUERY_H
#define CECS_QUERY_H

#include "cecs_view.h"
#include "cecs_world_components.h"
#include "query/cecs_query_descriptor.h"

void cecs_query_acquire(
    const cecs_world_components *const components,
    const cecs_query_descriptor *const descriptor,
    cecs_query_result *const out_result
);
// FIXME: come up with elegant place for allocators parameter
void cecs_query_acquire_mut(
    cecs_world_components *const components,
    const cecs_query_descriptor *const descriptor,
    cecs_allocator *const allocators, // XXX: NULLABLE!
    cecs_query_result *const out_result
);

void cecs_query_release(
    const cecs_world_components *const components,
    const cecs_query_descriptor *const descriptor,
    cecs_query_result *const result
);
void cecs_query_release_mut(
    cecs_world_components *const components,
    const cecs_query_descriptor *const descriptor,
    cecs_query_result *const result
);

#endif
