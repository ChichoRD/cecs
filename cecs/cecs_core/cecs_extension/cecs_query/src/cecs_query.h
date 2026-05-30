#ifndef CECS_QUERY_H
#define CECS_QUERY_H

#include <world/cecs_view.h>
#include <world/cecs_entity.h>
#include <stdint.h>
#include <stdlib.h>

// typedef union cecs_query_views {
//     const cecs_view *views;
//     const cecs_view_mut *views_mut;
//     const cecs_view_alloc *views_alloc;
// } cecs_query_views;
typedef cecs_view_unchecked cecs_query_view;
typedef cecs_query_view cecs_query_views[];
typedef union cecs_query_registry {
    const cecs_component_registry *registry;
    cecs_component_registry *registry_mut;
} cecs_query_registry;


typedef enum cecs_query_access {
    cecs_query_access_shared = 0,
    cecs_query_access_mut,
    // cecs_query_access_alloc,
} cecs_query_access;
typedef uint8_t cecs_query_access_value;

typedef enum cecs_query_match_type {
    cecs_query_match_type_all = 0,
    cecs_query_match_type_any,
    cecs_query_match_type_none_of,
    // [...]
} cecs_query_match_type;
typedef uint8_t cecs_query_match_value;


typedef struct cecs_query_term {
    cecs_query_view *views;
    size_t view_count;
    cecs_query_access_value access;
    cecs_query_match_value match;
} cecs_query_term;

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
