#ifndef COMPONENT_ITERATOR_H
#define COMPONENT_ITERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "../../types/macro_utils.h"
#include "../../containers/arena.h"
#include "component.h"

#define COMPONENTS_ALL_ID components_all
#define COMPONENTS_ANY_ID components_any
#define COMPONENTS_NONE_ID components_none

typedef TAGGED_UNION_STRUCT(
    component_search_group,
    components_type_info,
    COMPONENTS_ALL_ID,
    components_type_info,
    COMPONENTS_ANY_ID,
    components_type_info,
    COMPONENTS_NONE_ID
) component_search_group;

#define COMPONENTS_ALL(...) TAGGED_UNION_CREATE(COMPONENTS_ALL_ID, component_search_group, COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__))
#define COMPONENTS_ANY(...) TAGGED_UNION_CREATE(COMPONENTS_ANY_ID, component_search_group, COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__))
#define COMPONENTS_NONE(...) TAGGED_UNION_CREATE(COMPONENTS_NONE_ID, component_search_group, COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__))

typedef struct components_search_groups {
    const component_search_group *const groups;
    const size_t group_count;
} components_search_groups;

inline components_search_groups components_search_groups_create(
    const component_search_group *groups,
    size_t group_count
) {
    return (components_search_groups) {
        .groups = groups,
        .group_count = group_count
    };
}

#define COMPONENTS_SEARCH_GROUPS_CREATE(...) \
    components_search_groups_create( \
        ((component_search_group[]){ __VA_ARGS__ }), \
        sizeof((component_search_group[]){ __VA_ARGS__ }) / sizeof(component_search_group) \
    )



typedef struct component_iterator_descriptor {
    const world_components_checksum checksum;
    const world_components *const world_components;
    components_type_info_deconstruct;
    hibitset entities_bitset; 
} component_iterator_descriptor;

bool component_iterator_descriptor_copy_component_bitsets(
    const world_components *world_components,
    components_type_info components_type_info,
    hibitset bitsets_destination[]
) {
    bool intersection_empty = false;
    for (size_t i = 0; i < components_type_info.component_count; i++) {
        if (!world_components_has_storage(world_components, components_type_info.component_ids[i])) {
            intersection_empty = true;
            bitsets_destination[i] = hibitset_empty();
        } else {
            component_storage *storage = world_components_get_component_storage_unchecked(
                world_components,
                components_type_info.component_ids[i]
            );
            bitsets_destination[i] = storage->entity_bitset;
        }
    }
    return !intersection_empty;
}

size_t component_iterator_descriptor_append_sized_component_ids(
    const world_components *world_components,
    arena *iterator_temporary_arena,
    components_type_info components_type_info,
    list *sized_component_ids
) {
    for (size_t i = 0; i < components_type_info.component_count; i++) {
        if (
            world_components_has_storage(world_components, components_type_info.component_ids[i])
            && !component_storage_info(world_components_get_component_storage_unchecked(
                world_components,
                components_type_info.component_ids[i]
            )).is_unit_type_storage
        ) {
            LIST_ADD(component_id, sized_component_ids, iterator_temporary_arena, &components_type_info.component_ids[i]);
        }
    }

    return LIST_COUNT(component_id, sized_component_ids);
}

hibitset component_iterator_descriptor_get_search_groups_bitset(
    const world_components *world_components,
    arena *iterator_temporary_arena,
    components_search_groups search_groups,
    size_t max_component_count
) {
    hibitset *bitsets = calloc(max_component_count + 1, sizeof(hibitset));
    hibitset entities_bitset = hibitset_create(iterator_temporary_arena);
    for (size_t i = 0; i < search_groups.group_count; i++) {
        components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, search_groups.groups[i]);
        assert(max_component_count >= info.component_count && "component count is larger than max component count");
        
        bool component_bitsets_empty = !component_iterator_descriptor_copy_component_bitsets(
            world_components,
            info,
            bitsets
        );
        bool enties_bitset_empty = hibitset_is_empty(&entities_bitset);

        TAGGED_UNION_MATCH(search_groups.groups[i]) {
            case TAGGED_UNION_VARIANT(COMPONENTS_ALL_ID, component_search_group): {
                if (component_bitsets_empty) {
                    hibitset_unset_all(component_bitsets_empty);
                } else if (enties_bitset_empty) {
                    entities_bitset = hibitset_intersection(bitsets, info.component_count, iterator_temporary_arena);
                } else {
                    bitsets[info.component_count] = entities_bitset;
                    entities_bitset = hibitset_intersection(bitsets, info.component_count + 1, iterator_temporary_arena);
                }
                break;
            }

            case TAGGED_UNION_VARIANT(COMPONENTS_ANY_ID, component_search_group): {
                if (component_bitsets_empty)
                    break;

                if (enties_bitset_empty) {
                    entities_bitset = hibitset_union(bitsets, info.component_count, iterator_temporary_arena);
                } else {
                    bitsets[info.component_count] = entities_bitset;
                    entities_bitset = hibitset_union(bitsets, info.component_count + 1, iterator_temporary_arena);
                }
                break;
            }

            case TAGGED_UNION_VARIANT(COMPONENTS_NONE_ID, component_search_group): {
                if (component_bitsets_empty || enties_bitset_empty) {
                    break;
                }
                entities_bitset = hibitset_difference(&entities_bitset, bitsets, info.component_count, iterator_temporary_arena);
                break;
            }

            default: {
                assert(false && "unreachable: invalid component search group variant");
                exit(EXIT_FAILURE);
            }
        }
    }
    free(bitsets);

    return entities_bitset;
}

component_iterator_descriptor component_iterator_descriptor_create(
    const world_components *world_components,
    arena *iterator_temporary_arena,
    components_search_groups search_groups
) {
    list sized_component_ids = LIST_CREATE_WITH_CAPACITY(component_id, iterator_temporary_arena, search_groups.group_count);
    size_t sized_component_count = 0;
    size_t component_count = 0;
    size_t max_component_count = 0;
    for (size_t i = 0; i < search_groups.group_count; i++) {
        components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, search_groups.groups[i]);
        component_count += info.component_count;
        max_component_count = max(max_component_count, info.component_count);

        if (!TAGGED_UNION_IS(COMPONENTS_NONE_ID, component_search_group, search_groups.groups[i])) {        
            sized_component_count += component_iterator_descriptor_append_sized_component_ids(
                world_components,
                iterator_temporary_arena,
                info,
                &sized_component_ids
            );
        }
    }
    
    hibitset entities_bitset = component_iterator_descriptor_get_search_groups_bitset(
        world_components,
        iterator_temporary_arena,
        search_groups,
        max_component_count
    );
    component_iterator_descriptor descriptor = {
        .checksum = world_components->checksum,
        .world_components = world_components,
        .entities_bitset = entities_bitset,
        .components_type_info = (struct components_type_info) {
            .component_ids = sized_component_ids.elements,
            .component_count = sized_component_count
        }
    };

    return descriptor;
}


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
raw_iteration_handle_reference component_iterator_descriptor_allocate_handle(const component_iterator_descriptor descriptor) {
    return malloc(component_iteration_handle_size(descriptor.components_type_info));
}

raw_iteration_handle_reference component_iterator_descriptor_allocate_handle_in(arena *a, const component_iterator_descriptor descriptor) {
    return arena_alloc(a, component_iteration_handle_size(descriptor.components_type_info));
}


typedef struct component_iterator {
    component_iterator_descriptor descriptor;
    hibitset_iterator entities_iterator;
} component_iterator;

component_iterator component_iterator_create(component_iterator_descriptor descriptor) {
    assert(descriptor.checksum == descriptor.world_components->checksum
        && "Component iterator descriptor is invalid or outdated, please create a new one");
    hibitset_iterator iterator = hibitset_iterator_create_owned_at_first(descriptor.entities_bitset);
    if (!hibitset_iterator_current_is_set(&iterator)) {
        hibitset_iterator_next_set(&iterator);
    }
    return (component_iterator) {
        .descriptor = descriptor,
        .entities_iterator = iterator
    };
}
#define COMPONENT_ITERATOR_CREATE(world_components_ref, arena_ref, ...) \
    component_iterator_create(component_iterator_descriptor_create( \
        world_components_ref, \
        arena_ref, \
        COMPONENTS_SEARCH_GROUPS_CREATE(COMPONENTS_ALL(__VA_ARGS__)) \
    ))

#define COMPONENT_ITERATOR_CREATE_GROUPED(world_components_ref, arena_ref, ...) \
    component_iterator_create(component_iterator_descriptor_create( \
        world_components_ref, \
        arena_ref, \
        COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__) \
    ))

bool component_iterator_done(const component_iterator *it) {
    return hibitset_iterator_done(&it->entities_iterator);
}

entity_id component_iterator_current(const component_iterator *it, raw_iteration_handle_reference out_iteration_handle) {
    entity_id *handle = (entity_id *)out_iteration_handle;
    *handle = it->entities_iterator.current_bit_index;

    for (size_t i = 0; i < it->descriptor.component_count; i++) {
        component_storage *storage = world_components_get_component_storage_unchecked(
            it->descriptor.world_components,
            it->descriptor.component_ids[i]
        );
        
        ((raw_component_reference *)(handle + 1))[i] = component_storage_get_unchecked(
            storage,
            it->entities_iterator.current_bit_index,
            world_components_get_component_size_unchecked(it->descriptor.world_components, it->descriptor.component_ids[i])
        );
    }
    return it->entities_iterator.current_bit_index;
}

size_t component_iterator_next(component_iterator *it) {
    return hibitset_iterator_next_set(&it->entities_iterator);
}

#endif