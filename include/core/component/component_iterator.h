#ifndef COMPONENT_ITERATOR_H
#define COMPONENT_ITERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "../../types/macro_utils.h"
#include "../../containers/arena.h"
#include "component.h"

typedef void *raw_component_handle;
#define COMPONENT_HANDLE(identifier) CAT2(identifier, _component_handle)
#define COMPONENT_HANDLE_STRUCT(type) \
    struct COMPONENT_HANDLE(type) { \
        union { \
            raw_component_handle raw; \
            type *component; \
        }; \
    }


typedef struct component_iterator_descriptor {
    const world_components *const world_components;
    const component_id *const component_ids;
    const size_t component_count;
    hibitset entities_bitset;
} component_iterator_descriptor;

component_iterator_descriptor component_iterator_descriptor_create(
    const world_components *world_components,
    arena *iterator_temporary_arena,
    const component_id *const component_ids,
    size_t component_count) {
    hibitset *bitsets = calloc(component_count, sizeof(hibitset));
    for (size_t i = 0; i < component_count; i++) {
        bitsets[i] = LIST_GET(
            component_storage,
            &world_components->component_storages,
            component_ids[i]
        )->entity_bitset;
    }
    component_iterator_descriptor descriptor = {
        .world_components = world_components,
        .entities_bitset = hibitset_intersection(bitsets, component_count, iterator_temporary_arena),
        .component_ids = component_ids,
        .component_count = component_count
    };

    free(bitsets);
    return descriptor;
}


typedef struct component_iterator {
    component_iterator_descriptor descriptor;
    entity_id current_entity_id;
} component_iterator;

#define _PREPEND_UNDERSCORE(x) _##x
#define _COMPONENT_ITERATOR_FIELD(type) \
    COMPONENT_HANDLE_STRUCT(type) CAT2(type, _handle)
#define COMPONENT_ITERATOR(...) CAT(FIRST(__VA_ARGS__), MAP(_PREPEND_UNDERSCORE, COMMA, TAIL(__VA_ARGS__)), _component_iterator)
#define COMPONENT_ITERATOR_STRUCT(...) \
    struct COMPONENT_ITERATOR(__VA_ARGS__) { \
        component_iterator iterator; \
        MAP(_COMPONENT_ITERATOR_FIELD, SEMICOLON, __VA_ARGS__); \
    }

component_iterator *component_iterator_create(component_iterator_descriptor descriptor, arena *iterator_arena) {
    component_iterator *it = arena_alloc(
        iterator_arena,
        sizeof(component_iterator) + padding_between(component_iterator, void *) + sizeof(void *) * descriptor.component_count
    );
    return memcpy(it, &((component_iterator){ .descriptor = descriptor, .current_entity_id = 0 }), sizeof(component_iterator));
}
#define COMPONENT_ITERATOR_CREATE(world_components_ref, arena_ref, ...) \
    ((COMPONENT_ITERATOR_STRUCT(__VA_ARGS__) *) component_iterator_create(component_iterator_descriptor_create( \
        world_components_ref, \
        arena_ref, \
        COMPONENT_ID_ARRAY(__VA_ARGS__), \
        COMPONENT_COUNT(__VA_ARGS__) \
    ), arena_ref))

bool component_iterator_done(const component_iterator *it) {
    hibitset_iterator iterator = hibitset_iterator_create_at(&it->descriptor.entities_bitset, it->current_entity_id);
    return hibitset_iterator_done(&iterator);
}

void *component_iterator_current(const component_iterator *it) {
    for (size_t i = 0; i < it->descriptor.component_count; i++) {
        component_storage *storage = LIST_GET(
            component_storage,
            &it->descriptor.world_components->component_storages,
            it->descriptor.component_ids[i]
        );
        
        ((raw_component_handle *)(it+ 1))[i] = component_storage_get(
            storage,
            it->current_entity_id,
            world_components_get_component_size_unchecked(it->descriptor.world_components, it->descriptor.component_ids[i])
        );
    }
    return it;
}

size_t component_iterator_next(component_iterator *it) {
    hibitset_iterator iterator = hibitset_iterator_create_at(&it->descriptor.entities_bitset, it->current_entity_id);
    return (it->current_entity_id = hibitset_iterator_next_set(&iterator));
}


#endif