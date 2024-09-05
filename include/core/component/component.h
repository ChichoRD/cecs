#ifndef COMPONENT_H
#define COMPONENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "../../types/macro_utils.h"
#include "../../types/type_id.h"
#include "../../containers/tagged_union.h"
#include "../../containers/list.h"
#include "../../containers/bitset.h"
#include "../../containers/arena.h"
#include "entity/entity.h"
#include "component_storage.h"

typedef OPTION_STRUCT(size_t, component_size) component_size;
typedef struct world_components {
    arena storages_arena;
    arena components_arena;
    list component_storages;
    list component_sizes;
} world_components;

world_components world_components_create(size_t component_type_capacity) {
    return (world_components) {
        .storages_arena = arena_create_with_capacity(sizeof(component_storage) * component_type_capacity),
        .components_arena = arena_create(),
        .component_storages = list_create(),
        .component_sizes = list_create(),
    };
}

void world_components_free(world_components *wc) {
    arena_free(&wc->components_arena);
    arena_free(&wc->storages_arena);
    wc->component_storages = (list){0};
    wc->component_sizes = (list){0};
}

component_size *world_components_get_component_size(world_components *wc, component_id component_id) {
    return LIST_GET(component_size, &wc->component_sizes, component_id);
}
size_t world_components_get_component_size_unchecked(const world_components *wc, component_id component_id) {
    return OPTION_GET(component_size, *world_components_get_component_size(wc, component_id));
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
        size_t missing = component_id - LIST_COUNT(component_size, &wc->component_sizes) + 1;
        stored_size = LIST_APPEND_EMPTY(component_size, &wc->component_sizes, &wc->storages_arena, missing);
        *stored_size = OPTION_CREATE_SOME_STRUCT(component_size, size);
    } else {
        stored_size = world_components_get_component_size(wc, component_id);
        if (OPTION_IS_SOME(component_size, *stored_size)) {
            assert((OPTION_GET(component_size, *stored_size) == size) && "Component size mismatch");
        } else {
            *stored_size = OPTION_CREATE_SOME_STRUCT(component_size, size);
        }
    }

    if (missing_storages) {
        size_t missing = component_id - LIST_COUNT(component_storage, &wc->component_storages) + 1;
        // TODO: choosing
        component_storage s = component_storage_create_sparse(&wc->components_arena, 1, OPTION_GET(component_size, *stored_size));
        storage = memcpy(
            LIST_APPEND_EMPTY(component_storage, &wc->component_storages, &wc->storages_arena, missing),
            &s,
            sizeof(component_storage)
        );
    } else {
        storage = LIST_GET(component_storage, &wc->component_storages, component_id);
    }

    return component_storage_set(storage, &wc->components_arena, entity_id, component, OPTION_GET(component_size, *stored_size));
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
    if (!OPTION_IS_SOME(component_size, stored_size)) {
        return OPTION_CREATE_NONE_STRUCT(component);
    } else if (component_storage_has(storage, entity_id)) {
        return OPTION_CREATE_SOME_STRUCT(component, component_storage_get(storage, entity_id, OPTION_GET(component_size, stored_size)));
    } else {
        return OPTION_CREATE_NONE_STRUCT(component);
    }
}

void *world_components_get_component_unchecked(const world_components *wc, entity_id entity_id, component_id component_id) {
    return OPTION_GET(component, world_components_get_component(wc, entity_id, component_id));
}

optional_component world_components_remove_component(world_components *wc, entity_id entity_id, component_id component_id) {
    bool missing_sizes = component_id >= LIST_COUNT(component_size, &wc->component_sizes);
    bool missing_storages = component_id >= LIST_COUNT(component_storage, &wc->component_storages);
    if (missing_sizes != missing_storages) {
        assert(0 && "unreachable: component_sizes and component_storages are out of sync");
        exit(EXIT_FAILURE);
    }
    assert((!missing_sizes && !missing_storages) && "Component ID out of bounds");

    component_size stored_size = *world_components_get_component_size(wc, component_id);
    if (!OPTION_IS_SOME(component_size, stored_size)) {
        return OPTION_CREATE_NONE_STRUCT(component);
    } else {
        return component_storage_remove(
            LIST_GET(component_storage, &wc->component_storages, component_id),
            &wc->components_arena,
            entity_id,
            OPTION_GET(component_size, stored_size)
        );
    }
}

void *world_components_remove_component_unchecked(world_components *wc, entity_id entity_id, component_id component_id) {
    return OPTION_GET(component, world_components_remove_component(wc, entity_id, component_id));
}

#endif