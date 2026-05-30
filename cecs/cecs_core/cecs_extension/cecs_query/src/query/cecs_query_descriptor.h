#ifndef CECS_QUERY_DESCRIPTOR_H
#define CECS_QUERY_DESCRIPTOR_H

#include <world/cecs_view.h>
#include <world/registry/cecs_component.h>
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


typedef struct cecs_query_term_descriptor {
    const cecs_component_type *components;
    size_t component_count;
    cecs_query_access_value access;
    cecs_query_match_value match;
} cecs_query_term_descriptor;


#endif
