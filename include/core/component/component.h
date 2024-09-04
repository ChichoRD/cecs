#ifndef COMPONENT_H
#define COMPONENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "../../types/macro_utils.h"
#include "../containers/tagged_union.h"
#include "../containers/list.h"
#include "../containers/bitset.h"
#include "../containers/arena.h"
#include "../type_id.h"
#include "entity/entity.h"
#include "component_storage.h"

typedef uint64_t component_id;
static component_id component_id_count = 0;

typedef OPTION_STRUCT(size_t, size_t) component_size;
typedef struct world_components {
    arena components_arena;
    list component_storages;
    list component_sizes;
} world_components;

world_components world_components_create(arena *world_components_arena, size_t component_capacity) {
    return (world_components) {
        .components_arena = arena_create(),
        .component_storages = LIST_CREATE(component_storage, world_components_arena, 1),
        .component_sizes = LIST_CREATE(component_size, world_components_arena, 1),
    };
}

void *world_components_set_component(world_components *wc, entity_id entity_id, component_id component_id, void *component, size_t size) {
    component_size *stored_size = NULL;
    component_storage *storage = NULL;
    bool missing_sizes = component_id >= LIST_COUNT(component_size, &wc->component_sizes);
    bool missing_storages = component_id >= LIST_COUNT(component_storage, &wc->component_storages);
    if (missing_sizes != missing_storages) {
        assert(0 && "unreachable: component_sizes and component_storages are out of sync");
        exit(EXIT_FAILURE);
    }

    if (missing_sizes) {
        size_t missing = component_id - LIST_COUNT(component_size, &wc->component_sizes);
        stored_size = LIST_APPEND_EMPTY(component_size, &wc->component_sizes, &wc->components_arena, missing);
        *stored_size = OPTION_CREATE_SOME_STRUCT(size_t, size);
    } else {
        stored_size = LIST_GET(component_size, &wc->component_sizes, component_id);
        if (OPTION_IS_SOME(*stored_size)) {
            assert((OPTION_GET(size_t, *stored_size) == size) && "Component size mismatch");
        } else {
            *stored_size = OPTION_CREATE_SOME_STRUCT(size_t, size);
        }
    }

    if (missing_storages) {
        size_t missing = component_id - LIST_COUNT(component_storage, &wc->component_storages);
        stored_size = LIST_APPEND_EMPTY(component_storage, &wc->component_storages, &wc->components_arena, missing);
    } else {
        stored_size = LIST_GET(component_storage, &wc->component_storages, component_id);
    }

    return component_storage_set(storage, &wc->components_arena, entity_id, component, size);
}

optional_component world_components_get_component(const world_components *wc, entity_id entity_id, component_id component_id) {
    bool missing_sizes = component_id >= LIST_COUNT(component_size, &wc->component_sizes);
    bool missing_storages = component_id >= LIST_COUNT(component_storage, &wc->component_storages);
    if (missing_sizes != missing_storages) {
        assert(0 && "unreachable: component_sizes and component_storages are out of sync");
        exit(EXIT_FAILURE);
    }
    assert((!missing_sizes && !missing_storages) && "Component ID out of bounds");

    component_size stored_size = *LIST_GET(component_size, &wc->component_sizes, component_id);
    component_storage *storage = LIST_GET(component_storage, &wc->component_storages, component_id);
    if (!OPTION_IS_SOME(stored_size)) {
        return OPTION_CREATE_NONE_STRUCT(component);
    } else if (component_storage_has(storage, entity_id)) {
        return OPTION_CREATE_SOME_STRUCT(component, component_storage_get(storage, entity_id, OPTION_GET(size_t, stored_size)));
    } else {
        return OPTION_CREATE_NONE_STRUCT(component);
    }
}

optional_component world_components_remove_component(world_components *wc, entity_id entity_id, component_id component_id) {
    bool missing_sizes = component_id >= LIST_COUNT(component_size, &wc->component_sizes);
    bool missing_storages = component_id >= LIST_COUNT(component_storage, &wc->component_storages);
    if (missing_sizes != missing_storages) {
        assert(0 && "unreachable: component_sizes and component_storages are out of sync");
        exit(EXIT_FAILURE);
    }
    assert((!missing_sizes && !missing_storages) && "Component ID out of bounds");

    component_size stored_size = *LIST_GET(component_size, &wc->component_sizes, component_id);
    if (!OPTION_IS_SOME(stored_size)) {
        return OPTION_CREATE_NONE_STRUCT(component);
    } else {
        return component_storage_remove(
            LIST_GET(component_storage, &wc->component_storages, component_id),
            &wc->components_arena,
            entity_id,
            OPTION_GET(size_t, stored_size)
        );
    }
}

#endif