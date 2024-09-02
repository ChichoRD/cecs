#ifndef COMPONENT_STORAGE_H
#define COMPONENT_STORAGE_H

#include <assert.h>
#include <stdlib.h>
#include "../../containers/list.h"
#include "../../containers/bitset.h"
#include "../../containers/range.h"
#include "../../containers/arena.h"
#include "../../containers/tagged_union.h"
#include "../entity.h"
#include "component.h"

typedef void *get_component(const void *self, entity_id id, size_t size);
typedef void *set_component(const void *self, arena *a, entity_id id, void *component, size_t size);
typedef void *remove_component(const void *self, arena *a, entity_id id, size_t size);
typedef struct component_storage_header {
    get_component *get;
    set_component *set;
    remove_component *remove;
} component_storage_header;

typedef OPTION_STRUCT(size_t, component_index) option_component_index;

typedef struct component_storage {
    list components;
    list component_indices;
    hibitset entity_bitset;
    exclusive_range entity_id_range;
} component_storage;

component_storage component_storage_create(arena *a, size_t component_capacity, size_t component_size) {
    return (component_storage) {
        .components = list_create(a, component_size * component_capacity),
        .component_indices = list_create(a, sizeof(option_component_index) * component_capacity),
        .entity_bitset = hibitset_create(a),
        .entity_id_range = {0, 0},
    };
}

bool component_storage_has(const component_storage *cs, entity_id entity_id) {
    return hibitset_is_set(&cs->entity_bitset, entity_id);
}

bool component_storage_entity_id_in_range(const component_storage *cs, entity_id entity_id) {
    return exclusive_range_contains(cs->entity_id_range, entity_id);
}

exclusive_range component_storage_expand_indices(component_storage *cs, arena *a, entity_id entity_id) {
    if (exclusive_range_is_empty(cs->entity_id_range)) {
        assert(LIST_COUNT(option_component_index, &cs->component_indices) == 0 && "Component storage was not empty when entity was added");
        LIST_ADD(option_component_index, &cs->component_indices, a, &((option_component_index){0}));
        return (cs->entity_id_range = exclusive_range_singleton(entity_id));
    }

    word_range expanded_range = exclusive_range_from(
        range_union(cs->entity_id_range.range, exclusive_range_singleton(entity_id).range)
    );
    range_splitting splitting = exclusive_range_difference(
        expanded_range,
        cs->entity_id_range
    );

    exclusive_range range0 = exclusive_range_from(splitting.ranges[0]);
    exclusive_range range1 = exclusive_range_from(splitting.ranges[1]);
    if (!exclusive_range_is_empty(range0)) {
        size_t missing = exclusive_range_length(range0);
        option_component_index *buffer = calloc(missing, sizeof(option_component_index));
        LIST_INSERT_RANGE(option_component_index, &cs->component_indices, a, 0, buffer, missing);
        free(buffer);
    } else if (!exclusive_range_is_empty(range1)) {
        size_t missing = exclusive_range_length(range1);
        option_component_index *buffer = calloc(missing, sizeof(option_component_index));
        LIST_ADD_RANGE(option_component_index, &cs->component_indices, a, buffer, missing);
        free(buffer);
    }

    return (cs->entity_id_range = expanded_range);
}

void *component_storage_get(const component_storage *cs, entity_id id, size_t component_size) {
    assert(component_storage_has(cs, id) && "Entity does not have requested component");
    ssize_t list_index = (ssize_t)id - cs->entity_id_range.start;
    if (list_index < 0 || list_index >= LIST_COUNT(option_component_index, &cs->component_indices)) {
        assert(false && "unreachable: list_index not in bounds");
    }
    option_component_index index = *LIST_GET(option_component_index, &cs->component_indices, list_index);
    return list_get(&cs->components, OPTION_GET(component_index, index), component_size);
}
#define COMPONENT_STORAGE_GET(type, component_storage_ref, entity_id) \
    ((type *)component_storage_get(component_storage_ref, entity_id, sizeof(type)))


void *component_storage_set(component_storage *cs, arena *a, entity_id entity_id, void *component, size_t component_size) {
    if (!component_storage_entity_id_in_range(cs, entity_id)) {
        component_storage_expand_indices(cs, a, entity_id);
    }
    hibitset_set(&cs->entity_bitset, a, entity_id);
    ssize_t list_index = (ssize_t)entity_id - cs->entity_id_range.start;
    if (list_index < 0 || list_index >= LIST_COUNT(option_component_index, &cs->component_indices)) {
        assert(false && "unreachable: list_index not in bounds");
    }
    
    option_component_index *index = LIST_GET(option_component_index, &cs->component_indices, list_index);
    if (OPTION_HAS(*index)) {
        return list_set(&cs->components, OPTION_GET(component_index, *index), component, component_size);
    } else {
        *index = (option_component_index)OPTION_CREATE_SOME(component_index, list_count_of_size(&cs->components, component_size));
        return list_add(&cs->components, a, component, component_size);
    }
}
#define COMPONENT_STORAGE_SET(type, component_storage_ref, arena_ref, entity_id, component_ref) \
    ((type *)component_storage_set(component_storage_ref, arena_ref, entity_id, component_ref, sizeof(type)))

void component_storage_remove(component_storage *cs, arena *a, entity_id entity_id, size_t component_size) {
    if (!component_storage_has(cs, entity_id)) {
        return;
    }

    ssize_t list_index = (ssize_t)entity_id - cs->entity_id_range.start;
    if (list_index < 0 || list_index >= LIST_COUNT(option_component_index, &cs->component_indices)) {
        assert(false && "unreachable: list_index not in bounds");
    }
    option_component_index *removed_index = LIST_GET(option_component_index, &cs->component_indices, list_index);
    list_remove_swap_last(
        &cs->components,
        a,
        OPTION_GET(component_index, *removed_index),
        component_size
    );
    hibitset_unset(&cs->entity_bitset, a, entity_id);

    if (list_index != LIST_COUNT(option_component_index, &cs->component_indices) - 1) {
        for (
            hibitset_iterator it = hibitset_iterator_create_at_last(&cs->entity_bitset);
            !hibitset_iterator_done(&it);
            hibitset_iterator_previous_set(&it)
        ) {
            if (!component_storage_entity_id_in_range(cs, hibitset_iterator_current(&it)))
                continue;
            
            ssize_t current_index = (ssize_t)hibitset_iterator_current(&it) - cs->entity_id_range.start;
            option_component_index *index = LIST_GET(option_component_index, &cs->component_indices, current_index);
            if (OPTION_GET(component_index, *index) == list_count_of_size(&cs->components, component_size)) {
                *index = OPTION_CREATE_SOME_STRUCT(component_index, OPTION_GET(component_index, *removed_index));
                break;
            }
        }
    }
    *removed_index = OPTION_CREATE_NONE_STRUCT(component_index);
}
#define COMPONENT_STORAGE_REMOVE(type, component_storage_ref, arena_ref, entity_id) \
    component_storage_remove(component_storage_ref, arena_ref, entity_id, sizeof(type))

#endif