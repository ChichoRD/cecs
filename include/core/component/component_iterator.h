#ifndef COMPONENT_ITERATOR_H
#define COMPONENT_ITERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "../../types/macro_utils.h"
#include "../../containers/arena.h"
#include "component.h"


typedef struct component_iterator_descriptor {
    const world_components *const world_components;
    components_type_info_deconstruct;
    hibitset entities_bitset; 
} component_iterator_descriptor;

component_iterator_descriptor component_iterator_descriptor_create(
    const world_components *world_components,
    arena *iterator_temporary_arena,
    components_type_info components_type_info
) {
    size_t component_count = components_type_info.component_count;
    list sized_component_ids = LIST_CREATE_WITH_CAPACITY(component_id, iterator_temporary_arena, component_count);

    hibitset *bitsets = calloc(component_count, sizeof(hibitset));
    bool intersection_empty = false;
    for (size_t i = 0; i < component_count; i++) {
        if (!world_components_has_storage(world_components, components_type_info.component_ids[i])) {
            intersection_empty = true;
            bitsets[i] = hibitset_empty();
        } else {
            component_storage *storage = world_components_get_component_storage_unchecked(
                world_components,
                components_type_info.component_ids[i]
            );
            bitsets[i] = storage->entity_bitset;
            if (!component_storage_info(storage).is_unit_type_storage) {
                LIST_ADD(component_id, &sized_component_ids, iterator_temporary_arena, &components_type_info.component_ids[i]);
            }
        }
        
    }

    component_iterator_descriptor descriptor = {
        .world_components = world_components,
        .entities_bitset = intersection_empty
            ? hibitset_empty()
            : hibitset_intersection(bitsets, component_count, iterator_temporary_arena),
        .components_type_info = (struct components_type_info) {
            .component_ids = sized_component_ids.elements,
            .component_count = LIST_COUNT(component_id, &sized_component_ids)
        }
    };

    free(bitsets);
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
        COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__) \
    ))

bool component_iterator_done(const component_iterator *it) {
    return hibitset_iterator_done(&it->entities_iterator);
}

entity_id component_iterator_current(const component_iterator *it, raw_iteration_handle_reference out_iteration_handle) {
    entity_id *handle = (entity_id *)out_iteration_handle;
    *handle = it->entities_iterator.current_bit_index;

    for (size_t i = 0; i < it->descriptor.component_count; i++) {
        component_storage *storage = LIST_GET(
            component_storage,
            &it->descriptor.world_components->component_storages,
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