#ifndef CECS_COMPONENT_ITERATOR_H
#define CECS_COMPONENT_ITERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "../../types/cecs_macro_utils.h"
#include "../../containers/cecs_arena.h"
#include "cecs_component.h"

#define CECS_COMPONENTS_ALL_ID components_all
#define CECS_COMPONENTS_ANY_ID components_any
#define CECS_COMPONENTS_NONE_ID components_none

#define CECS_COMPONENTS_OR_ALL_ID components_or_all
#define CECS_COMPONENTS_AND_ANY_ID components_and_any

typedef CECS_UNION_STRUCT(
    cecs_components_search_group,
    cecs_components_type_info,
    CECS_COMPONENTS_ALL_ID,
    cecs_components_type_info,
    CECS_COMPONENTS_ANY_ID,
    cecs_components_type_info,
    CECS_COMPONENTS_NONE_ID,
    cecs_components_type_info,
    CECS_COMPONENTS_OR_ALL_ID,
    cecs_components_type_info,
    CECS_COMPONENTS_AND_ANY_ID
) cecs_components_search_group;

#define CECS_COMPONENTS_SEARCH_GROUP_CREATE(components_type_info, variant) \
    ((cecs_components_search_group)CECS_UNION_CREATE(variant, cecs_components_search_group, components_type_info))

#define CECS_COMPONENTS_ALL(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), CECS_COMPONENTS_ALL_ID)
#define CECS_COMPONENTS_ANY(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), CECS_COMPONENTS_ANY_ID)
#define CECS_COMPONENTS_NONE(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), CECS_COMPONENTS_NONE_ID)
#define CECS_COMPONENTS_OR_ALL(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), CECS_COMPONENTS_OR_ALL_ID)
#define CECS_COMPONENTS_AND_ANY(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), CECS_COMPONENTS_AND_ANY_ID)

#define CECS_COMPONENTS_ALL_IDS(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), CECS_COMPONENTS_ALL_ID)
#define CECS_COMPONENTS_ANY_IDS(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), CECS_COMPONENTS_ANY_ID)
#define CECS_COMPONENTS_NONE_IDS(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), CECS_COMPONENTS_NONE_ID)
#define CECS_COMPONENTS_OR_ALL_IDS(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), CECS_COMPONENTS_OR_ALL_ID)
#define CECS_COMPONENTS_AND_ANY_IDS(...) CECS_COMPONENTS_SEARCH_GROUP_CREATE(CECS_COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), CECS_COMPONENTS_AND_ANY_ID)


typedef struct cecs_components_search_groups {
    cecs_components_search_group *groups;
    size_t group_count;
} cecs_components_search_groups;

static inline cecs_components_search_groups cecs_components_search_groups_create(
    cecs_components_search_group *groups,
    size_t group_count
) {
    return (cecs_components_search_groups) {
        .groups = groups,
        .group_count = group_count
    };
}

#define CECS_COMPONENTS_SEARCH_GROUPS_CREATE_RAW(...) \
    cecs_components_search_groups_create( \
        ((cecs_components_search_group[]){ __VA_ARGS__ }), \
        sizeof((cecs_components_search_group[]){ __VA_ARGS__ }) / sizeof(cecs_components_search_group) \
    )

#define CECS_COMPONENTS_SEARCH_GROUP_DEFAULT_IGNORED \
    CECS_COMPONENTS_NONE(cecs_is_prefab)

#define CECS_COMPONENTS_SEARCH_GROUPS_CREATE(...) \
    CECS_COMPONENTS_SEARCH_GROUPS_CREATE_RAW(CECS_COMPONENTS_SEARCH_GROUP_DEFAULT_IGNORED, __VA_ARGS__)



typedef struct cecs_component_iterator_descriptor {
    const cecs_world_components_checksum checksum;
    const cecs_world_components *const world_components;
    cecs_components_type_info_deconstruct;
    cecs_hibitset entities_bitset;
} cecs_component_iterator_descriptor;

bool cecs_component_iterator_descriptor_copy_component_bitsets(
    const cecs_world_components *world_components,
    cecs_components_type_info components_type_info,
    cecs_hibitset bitsets_destination[]
);

size_t cecs_component_iterator_descriptor_append_sized_component_ids(
    const cecs_world_components *world_components,
    cecs_arena *iterator_temporary_arena,
    cecs_components_type_info components_type_info,
    cecs_dynamic_array *sized_component_ids
);

cecs_hibitset cecs_component_iterator_descriptor_get_search_groups_bitset(
    const cecs_world_components *world_components,
    cecs_arena *iterator_temporary_arena,
    cecs_components_search_groups search_groups,
    size_t max_component_count
);

cecs_component_iterator_descriptor cecs_component_iterator_descriptor_create(
    const cecs_world_components *world_components,
    cecs_arena *iterator_temporary_arena,
    cecs_components_search_groups search_groups
);


#define _CECS_PREPEND_UNDERSCORE(x) _##x
#define _CECS_COMPONENT_ITERATION_HANDLE_FIELD(type) \
    type *CECS_CAT2(type, _component)
#define CECS_COMPONENT_ITERATION_HANDLE(...) \
    CECS_CAT(CECS_FIRST(__VA_ARGS__), CECS_MAP(_CECS_PREPEND_UNDERSCORE, CECS_COMMA, CECS_TAIL(__VA_ARGS__)), _component_iteration_handle)
#define CECS_COMPONENT_ITERATION_HANDLE_STRUCT(...) \
    struct CECS_COMPONENT_ITERATION_HANDLE(__VA_ARGS__) { \
        cecs_entity_id entity_id; \
        CECS_MAP(_CECS_COMPONENT_ITERATION_HANDLE_FIELD, CECS_SEMICOLON, __VA_ARGS__); \
    }



typedef void *cecs_raw_component_reference;
static inline size_t cecs_component_iteration_handle_size(cecs_components_type_info components_type_info) {
    return sizeof(cecs_entity_id)
        + cecs_padding_between(cecs_entity_id, cecs_raw_component_reference)
        + sizeof(cecs_raw_component_reference) * components_type_info.component_count;
}

typedef void *raw_iteration_handle_reference;
raw_iteration_handle_reference cecs_component_iterator_descriptor_allocate_handle(const cecs_component_iterator_descriptor descriptor);

raw_iteration_handle_reference cecs_component_iterator_descriptor_allocate_handle_in(cecs_arena *a, const cecs_component_iterator_descriptor descriptor);


typedef struct cecs_component_iterator {
    cecs_component_iterator_descriptor descriptor;
    cecs_hibitset_iterator entities_iterator;
    cecs_entity_id_range entity_range;
} cecs_component_iterator;

cecs_component_iterator cecs_component_iterator_create(cecs_component_iterator_descriptor descriptor);
#define CECS_COMPONENT_ITERATOR_CREATE_GROUPED(world_components_ref, arena_ref, ...) \
    cecs_component_iterator_create(cecs_component_iterator_descriptor_create( \
        world_components_ref, \
        arena_ref, \
        CECS_COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__) \
    ))
#define CECS_COMPONENT_ITERATOR_CREATE(world_components_ref, arena_ref, ...) \
    CECS_COMPONENT_ITERATOR_CREATE_GROUPED(world_components_ref, arena_ref, CECS_COMPONENTS_ALL(__VA_ARGS__))

cecs_component_iterator cecs_component_iterator_create_ranged(cecs_component_iterator_descriptor descriptor, cecs_entity_id_range range);
#define CECS_COMPONENT_ITERATOR_CREATE_RANGED_GROUPED(world_components_ref, arena_ref, entity_id_range0, ...) \
    cecs_component_iterator_create_ranged(cecs_component_iterator_descriptor_create( \
        world_components_ref, \
        arena_ref, \
        CECS_COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__) \
    ), entity_id_range0)
#define CECS_COMPONENT_ITERATOR_CREATE_RANGED(world_components_ref, arena_ref, entity_id_range0, ...) \
    CECS_COMPONENT_ITERATOR_CREATE_RANGED_GROUPED(world_components_ref, arena_ref, entity_id_range0, CECS_COMPONENTS_ALL(__VA_ARGS__))

bool cecs_component_iterator_done(const cecs_component_iterator *it);

cecs_entity_id cecs_component_iterator_current(const cecs_component_iterator *it, raw_iteration_handle_reference out_iteration_handle);

size_t cecs_component_iterator_next(cecs_component_iterator *it);

#endif