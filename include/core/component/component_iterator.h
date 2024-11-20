#ifndef COMPONENT_ITERATOR_H
#define COMPONENT_ITERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "../../types/macro_utils.h"
#include "../../containers/cecs_arena.h"
#include "component.h"

#define COMPONENTS_ALL_ID components_all
#define COMPONENTS_ANY_ID components_any
#define COMPONENTS_NONE_ID components_none

#define COMPONENTS_OR_ALL_ID components_or_all
#define COMPONENTS_AND_ANY_ID components_and_any

typedef TAGGED_UNION_STRUCT(
    components_search_group,
    components_type_info,
    COMPONENTS_ALL_ID,
    components_type_info,
    COMPONENTS_ANY_ID,
    components_type_info,
    COMPONENTS_NONE_ID,
    components_type_info,
    COMPONENTS_OR_ALL_ID,
    components_type_info,
    COMPONENTS_AND_ANY_ID
) components_search_group;

#define COMPONENTS_SEARCH_GROUP_CREATE(components_type_info, variant) \
    ((components_search_group)TAGGED_UNION_CREATE(variant, components_search_group, components_type_info))

#define COMPONENTS_ALL(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), COMPONENTS_ALL_ID)
#define COMPONENTS_ANY(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), COMPONENTS_ANY_ID)
#define COMPONENTS_NONE(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), COMPONENTS_NONE_ID)
#define COMPONENTS_OR_ALL(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), COMPONENTS_OR_ALL_ID)
#define COMPONENTS_AND_ANY(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__), COMPONENTS_AND_ANY_ID)

#define COMPONENTS_ALL_IDS(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), COMPONENTS_ALL_ID)
#define COMPONENTS_ANY_IDS(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), COMPONENTS_ANY_ID)
#define COMPONENTS_NONE_IDS(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), COMPONENTS_NONE_ID)
#define COMPONENTS_OR_ALL_IDS(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), COMPONENTS_OR_ALL_ID)
#define COMPONENTS_AND_ANY_IDS(...) COMPONENTS_SEARCH_GROUP_CREATE(COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__), COMPONENTS_AND_ANY_ID)


typedef struct components_search_groups {
    components_search_group *groups;
    size_t group_count;
} components_search_groups;

inline components_search_groups components_search_groups_create(
    components_search_group *groups,
    size_t group_count
) {
    return (components_search_groups) {
        .groups = groups,
        .group_count = group_count
    };
}

#define COMPONENTS_SEARCH_GROUPS_CREATE_RAW(...) \
    components_search_groups_create( \
        ((components_search_group[]){ __VA_ARGS__ }), \
        sizeof((components_search_group[]){ __VA_ARGS__ }) / sizeof(components_search_group) \
    )

#define COMPONENTS_SEARCH_GROUP_DEFAULT_IGNORED \
    COMPONENTS_NONE(is_prefab)

#define COMPONENTS_SEARCH_GROUPS_CREATE(...) \
    COMPONENTS_SEARCH_GROUPS_CREATE_RAW(COMPONENTS_SEARCH_GROUP_DEFAULT_IGNORED, __VA_ARGS__)



typedef struct component_iterator_descriptor {
    const world_components_checksum checksum;
    const world_components *const world_components;
    components_type_info_deconstruct;
    cecs_hibitset entities_bitset;
} component_iterator_descriptor;

bool component_iterator_descriptor_copy_component_bitsets(
    const world_components *world_components,
    components_type_info components_type_info,
    cecs_hibitset bitsets_destination[]
);

size_t component_iterator_descriptor_append_sized_component_ids(
    const world_components *world_components,
    cecs_arena *iterator_temporary_arena,
    components_type_info components_type_info,
    list *sized_component_ids
);

cecs_hibitset component_iterator_descriptor_get_search_groups_bitset(
    const world_components *world_components,
    cecs_arena *iterator_temporary_arena,
    components_search_groups search_groups,
    size_t max_component_count
);

component_iterator_descriptor component_iterator_descriptor_create(
    const world_components *world_components,
    cecs_arena *iterator_temporary_arena,
    components_search_groups search_groups
);


#define _PREPEND_UNDERSCORE(x) _##x
#define _COMPONENT_ITERATION_HANDLE_FIELD(type) \
    type *CAT2(type, _component)
#define COMPONENT_ITERATION_HANDLE(...) \
    CAT(FIRST(__VA_ARGS__), MAP(_PREPEND_UNDERSCORE, COMMA, TAIL(__VA_ARGS__)), _component_iteration_handle)
#define COMPONENT_ITERATION_HANDLE_STRUCT(...) \
    struct COMPONENT_ITERATION_HANDLE(__VA_ARGS__) { \
        entity_id entity_id; \
        MAP(_COMPONENT_ITERATION_HANDLE_FIELD, SEMICOLON, __VA_ARGS__); \
    }



typedef void *raw_component_reference;
inline size_t component_iteration_handle_size(components_type_info components_type_info) {
    return sizeof(entity_id)
        + padding_between(entity_id, raw_component_reference)
        + sizeof(raw_component_reference) * components_type_info.component_count;
}

typedef void *raw_iteration_handle_reference;
raw_iteration_handle_reference component_iterator_descriptor_allocate_handle(const component_iterator_descriptor descriptor);

raw_iteration_handle_reference component_iterator_descriptor_allocate_handle_in(cecs_arena *a, const component_iterator_descriptor descriptor);


typedef struct component_iterator {
    component_iterator_descriptor descriptor;
    cecs_hibitset_iterator entities_iterator;
    entity_id_range entity_range;
} component_iterator;

component_iterator component_iterator_create(component_iterator_descriptor descriptor);
#define COMPONENT_ITERATOR_CREATE_GROUPED(world_components_ref, arena_ref, ...) \
    component_iterator_create(component_iterator_descriptor_create( \
        world_components_ref, \
        arena_ref, \
        COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__) \
    ))
#define COMPONENT_ITERATOR_CREATE(world_components_ref, arena_ref, ...) \
    COMPONENT_ITERATOR_CREATE_GROUPED(world_components_ref, arena_ref, COMPONENTS_ALL(__VA_ARGS__))

component_iterator component_iterator_create_ranged(component_iterator_descriptor descriptor, entity_id_range range);
#define COMPONENT_ITERATOR_CREATE_RANGED_GROUPED(world_components_ref, arena_ref, entity_id_range0, ...) \
    component_iterator_create_ranged(component_iterator_descriptor_create( \
        world_components_ref, \
        arena_ref, \
        COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__) \
    ), entity_id_range0)
#define COMPONENT_ITERATOR_CREATE_RANGED(world_components_ref, arena_ref, entity_id_range0, ...) \
    COMPONENT_ITERATOR_CREATE_RANGED_GROUPED(world_components_ref, arena_ref, entity_id_range0, COMPONENTS_ALL(__VA_ARGS__))

bool component_iterator_done(const component_iterator *it);

entity_id component_iterator_current(const component_iterator *it, raw_iteration_handle_reference out_iteration_handle);

size_t component_iterator_next(component_iterator *it);

#endif