#ifndef CECS_QUERY_H
#define CECS_QUERY_H

#include <world/cecs_view.h>
#include <world/cecs_entity.h>
#include <stdint.h>
#include <stdlib.h>
#include "query/cecs_query_descriptor.h"

typedef struct cecs_query_term {
    cecs_query_view *views;
    size_t view_count;
    cecs_query_access_value access;
    cecs_query_match_value match;
} cecs_query_term;
inline cecs_query_term cecs_query_term_create(
    cecs_query_view *const views,
    const size_t view_count,
    const cecs_query_access access,
    const cecs_query_match_type match
) {
    return (cecs_query_term){
        .views = views,
        .view_count = view_count,
        .access = (cecs_query_access_value)access,
        .match = (cecs_query_match_value)match
    };
}
cecs_query_term cecs_query_term_acquire(
    const cecs_query_term_descriptor descriptor,
    const cecs_world_components *const components,
    // FIXME: if query (terms) can own the views, they need to store the borrow too
    cecs_query_view *const out_views,
    const size_t out_view_count
);
cecs_query_term cecs_query_term_release(cecs_query_term *const term);

size_t cecs_query_term_find_storages(
    const cecs_query_term term,
    const cecs_world_components *const components,
    const cecs_component_registry **const out_registries,
    const size_t out_registry_count
);
size_t cecs_query_term_find_storages_mut(
    const cecs_query_term term,
    cecs_world_components *const components,
    cecs_component_registry **const out_registries,
    const size_t out_registry_count
);
bool cecs_query_term_match(
    const cecs_query_match_value match,
    const cecs_component_registry *const *const registries,
    const size_t registry_count,
    const cecs_entity_index entity
);
size_t cecs_query_term_count_min(
    const cecs_query_term term,
    const cecs_component_registry *const *const registries,
    const size_t registry_count
);
size_t cecs_query_term_count_min_mut(
    const cecs_query_term term,
    const cecs_query_registry *const registries,
    const size_t registry_count
);

typedef struct cecs_query_term_slice {
    size_t start_inclusive;
    size_t end_exclusive;
} cecs_query_term_slice;
bool cecs_query_term_match_slice(
    const cecs_query_match_value match,
    const cecs_component_registry *const *const registries,
    const size_t registry_count,
    const cecs_entity_index entity,
    const cecs_query_term_slice slice
);


typedef struct cecs_query {
    cecs_query_term *terms;
    size_t term_count;
} cecs_query;
inline cecs_query cecs_query_create(cecs_query_term *const terms, const size_t term_count) {
    return (cecs_query){
        .terms = terms,
        .term_count = term_count
    };
} 

size_t cecs_query_find_storages(
    const cecs_query query,
    const cecs_world_components *const components,
    const cecs_component_registry **const out_registries,
    const size_t out_registry_count
);
size_t cecs_query_find_storages_mut(
    const cecs_query query,
    cecs_world_components *const components,
    cecs_query_registry *const out_registries,
    const size_t out_registry_count
);
bool cecs_query_match(
    const cecs_query query,
    const cecs_component_registry *const *const registries,
    const size_t registry_count,
    const cecs_entity_index entity
);
size_t cecs_query_count_min(
    const cecs_query query,
    const cecs_component_registry *const *const registries,
    const size_t registry_count
);
size_t cecs_query_count_min_mut(
    const cecs_query query,
    const cecs_query_registry *const registries,
    const size_t registry_count
);

bool cecs_query_match_slices(
    const cecs_query query,
    const cecs_component_registry *const *const registries,
    const size_t registry_count,
    const cecs_entity_index entity,
    const cecs_query_term_slice *const slices,
    const size_t slice_count
);
bool cecs_query_match_slices_explicit(
    const cecs_query query,
    const cecs_component_registry *const *const registries,
    const size_t registry_count,
    const cecs_entity_index entity,
    const cecs_query_term_slice *const slices,
    const size_t slice_count
);

#endif // CECS_QUERY_H
