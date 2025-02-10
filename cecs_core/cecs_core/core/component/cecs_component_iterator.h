#ifndef CECS_COMPONENT_ITERATOR_H
#define CECS_COMPONENT_ITERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "../../types/cecs_macro_utils.h"
#include "../../containers/cecs_arena.h"
#include "cecs_component.h"

typedef enum cecs_component_group_search {
    cecs_component_group_search_all,
    cecs_component_group_search_any,
    cecs_component_group_search_none,
    cecs_component_group_search_or_all,
    cecs_component_group_search_and_any
} cecs_component_group_search;
typedef uint8_t cecs_component_group_search_mode;

typedef struct cecs_component_iteration_group {
    cecs_component_id *components;
    size_t component_count; 
    cecs_component_access_mode access;
    cecs_component_group_search_mode search_mode;
} cecs_component_iteration_group;
typedef struct cecs_component_iterator_descriptor {
    cecs_entity_id_range entity_range;
    cecs_component_iteration_group *groups;
    size_t group_count;
} cecs_component_iterator_descriptor;


typedef struct cecs_component_iterator_descriptor_flat {
    cecs_entity_id_range entity_range;
    cecs_component_id *components;
} cecs_component_iterator_descriptor_flat;

typedef union cecs_component_iterator_descriptor_union {
    cecs_component_iterator_descriptor groupped;
    cecs_component_iterator_descriptor_flat flattened;
} cecs_component_iterator_descriptor_union;

// TODO: decide leading iterator
typedef enum cecs_component_iterator_status {
    cecs_component_iterator_status_none = 0,
    cecs_component_iterator_status_iter_checked = 1 << 0,
} cecs_component_iterator_status;
typedef uint8_t cecs_component_iterator_status_flags;

typedef struct cecs_component_iterator {
    cecs_component_iterator_descriptor_union descriptor;
    cecs_hibitset_iterator entities_iterator;
    cecs_world_components_checksum creation_checksum;
    cecs_world_components *world_components;
    size_t component_count;
    cecs_component_iterator_status_flags flags;
} cecs_component_iterator;

cecs_component_iterator cecs_component_iterator_create(
    const cecs_component_iterator_descriptor descriptor,
    cecs_world_components *world_components,
    cecs_arena *iterator_temporary_arena
);

bool cecs_component_iterator_can_begin_iter(const cecs_component_iterator *it);
cecs_entity_id cecs_component_iterator_begin_iter(cecs_component_iterator *it, cecs_arena *iterator_temporary_arena);

cecs_component_iterator cecs_component_iterator_create_and_begin_iter(
    cecs_component_iterator_descriptor descriptor,
    cecs_world_components *world_components,
    cecs_arena *iterator_temporary_arena
);

bool cecs_component_iterator_done(const cecs_component_iterator *it);
cecs_entity_id cecs_component_iterator_current(const cecs_component_iterator *it, void *out_component_handles[]);
size_t cecs_component_iterator_next(cecs_component_iterator *it);

void cecs_component_iterator_end_iter(cecs_component_iterator *it);

#define _CECS_PREPEND_UNDERSCORE(x) _##x
#define _CECS_COMPONENT_ITERATION_HANDLE_FIELD(type) \
    type *CECS_COMPONENT(type)
#define CECS_COMPONENT_ITERATION_HANDLE(...) \
    CECS_CAT(CECS_FIRST(__VA_ARGS__), CECS_MAP(_CECS_PREPEND_UNDERSCORE, CECS_COMMA, CECS_TAIL(__VA_ARGS__)), _component_iteration_handle)

#define CECS_COMPONENT_ITERATION_HANDLE_STRUCT(...) \
    struct CECS_COMPONENT_ITERATION_HANDLE(__VA_ARGS__) { \
        CECS_MAP(_CECS_COMPONENT_ITERATION_HANDLE_FIELD, CECS_SEMICOLON, __VA_ARGS__); \
    }

#define _CECS_COMPONENT_ITERATION_HANDLE_FIELD_ALIAS(type, alias) \
    type *alias
#define CECS_COMPONENT_ITERATION_HANDLE_ALIAS_STRUCT(...) \
    struct CECS_COMPONENT_ITERATION_HANDLE(__VA_ARGS__) { \
        CECS_MAP_PAIRS(_CECS_COMPONENT_ITERATION_HANDLE_FIELD_ALIAS, CECS_SEMICOLON, __VA_ARGS__); \
    }

#define CECS_COMPONENT_GROUP(access_mode, component_search, ...) \
    (cecs_component_iteration_group){ \
        .components = CECS_COMPONENT_ID_ARRAY(__VA_ARGS__), \
        .component_count = CECS_COMPONENT_COUNT(__VA_ARGS__), \
        .access = access_mode, \
        .search_mode = component_search \
    }
#define CECS_COMPONENT_GROUP_FROM_IDS(access_mode, component_search, ...) \
    (cecs_component_iteration_group){ \
        .components = (cecs_component_id[]){ __VA_ARGS__ }, \
        .component_count = (sizeof((cecs_component_id[]){ __VA_ARGS__ }) / sizeof(cecs_component_id)), \
        .access = access_mode, \
        .search_mode = component_search \
    }
#define CECS_COMPONENT_GROUP_DEFAULT_EXCLUDED \
    CECS_COMPONENT_GROUP(cecs_component_access_ignore, cecs_component_group_search_none, \
        cecs_is_prefab \
    )

#define CECS_COMPONENT_ITERATOR_DESCRIPTOR_CREATE_GROUPPED_RAW(...) \
    (cecs_component_iterator_descriptor){ \
        .entity_range = {0, PTRDIFF_MAX}, \
        .groups = (cecs_component_iteration_group[]){ __VA_ARGS__ }, \
        .group_count = (sizeof((cecs_component_iteration_group[]){ __VA_ARGS__ }) / sizeof(cecs_component_iteration_group)) \
    }
#define CECS_COMPONENT_ITERATOR_DESCRIPTOR_CREATE_GROUPPED(...) \
    CECS_COMPONENT_ITERATOR_DESCRIPTOR_CREATE_GROUPPED_RAW( \
        (CECS_COMPONENT_GROUP_DEFAULT_EXCLUDED), \
        __VA_ARGS__ \
    )

#define CECS_COMPONENT_ITERATOR_CREATE_GROUPPED(world_components_reference, iterator_temporary_arena_reference, ...) \
    cecs_component_iterator_create( \
        CECS_COMPONENT_ITERATOR_DESCRIPTOR_CREATE_GROUPPED(__VA_ARGS__), \
        world_components_reference, \
        iterator_temporary_arena_reference \
    )

#define CECS_COMPONENT_ITERATOR_CREATE(world_components_reference, iterator_temporary_arena_reference, ...) \
    CECS_COMPONENT_ITERATOR_CREATE_GROUPPED( \
        world_components_reference, iterator_temporary_arena_reference, \
        CECS_COMPONENTS_GROUP(cecs_component_access_inmmutable, cecs_component_group_search_all, __VA_ARGS__) \
    )
#define CECS_COMPONENT_ITERATOR_CREATE_MUT(world_components_reference, iterator_temporary_arena_reference, ...) \
    CECS_COMPONENT_ITERATOR_CREATE_GROUPPED( \
        world_components_reference, iterator_temporary_arena_reference, \
        CECS_COMPONENTS_GROUP(cecs_component_access_mutable, cecs_component_group_search_all, __VA_ARGS__) \
    )

#endif