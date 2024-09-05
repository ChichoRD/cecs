#ifndef COMPONENT_H
#define COMPONENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "../../types/macro_utils.h"
#include "../../types/type_id.h"
#include "../../containers/tagged_union.h"
#include "../../containers/sparse_set.h"
#include "../../containers/bitset.h"
#include "../../containers/arena.h"
#include "entity/entity.h"
#include "component_storage.h"

typedef struct world_components {
    arena storages_arena;
    arena components_arena;
    sparse_set component_storages;
    sparse_set component_sizes;
} world_components;

world_components world_components_create(size_t component_type_capacity) {
    return (world_components) {
        .storages_arena = arena_create_with_capacity(sizeof(component_storage) * component_type_capacity),
        .components_arena = arena_create(),
        .component_storages = sparse_set_create(),
        .component_sizes = sparse_set_create(),
    };
}

void world_components_free(world_components *wc) {
    arena_free(&wc->components_arena);
    arena_free(&wc->storages_arena);
    wc->component_storages = (sparse_set){0};
    wc->component_sizes = (sparse_set){0};
}

typedef OPTION_STRUCT(size_t *, optional_component_size) optional_component_size;
optional_component_size world_components_get_component_size(world_components *wc, component_id component_id) {
    return OPTION_MAP_REFERENCE_STRUCT(
        optional_element,
        SPARSE_SET_GET(size_t, &wc->component_sizes, component_id),
        optional_component_size
    );
}
size_t world_components_get_component_size_unchecked(const world_components *wc, component_id component_id) {
    return *OPTION_GET(optional_component_size, world_components_get_component_size(wc, component_id));
}

typedef OPTION_STRUCT(component_storage *, optional_component_storage) optional_component_storage;
optional_component_storage world_components_get_component_storage(world_components *wc, component_id component_id) {
    return OPTION_MAP_REFERENCE_STRUCT(
        optional_element,
        SPARSE_SET_GET(component_storage, &wc->component_storages, component_id),
        optional_component_storage
    );
}
void *world_components_set_component(world_components *wc, entity_id entity_id, component_id component_id, void *component, size_t size) {
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    if (OPTION_IS_SOME(optional_component_size, stored_size)) {
        assert((*OPTION_GET(optional_component_size, stored_size) == size) && "Component size mismatch");
    } else {
        stored_size = OPTION_CREATE_SOME_STRUCT(
            optional_component_size,
            SPARSE_SET_SET(size_t, &wc->component_sizes, &wc->storages_arena, component_id, &size)
        );
    }

    optional_component_storage storage = world_components_get_component_storage(wc, component_id);
    if (OPTION_IS_SOME(optional_component_storage, storage)) {
        return component_storage_set(
            OPTION_GET(optional_component_storage, storage),
            &wc->components_arena,
            entity_id,
            component,
            size
        );
    } else {
        // TODO: choosing
        component_storage s = component_storage_create_sparse(&wc->components_arena, 1, size);
        return component_storage_set(
            SPARSE_SET_SET(component_storage, &wc->component_storages, &wc->storages_arena, component_id, &s),
            &wc->components_arena,
            entity_id,
            component,
            size
        );
    }
}

optional_component world_components_get_component(const world_components *wc, entity_id entity_id, component_id component_id) {
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    optional_component_storage storage = world_components_get_component_storage(wc, component_id);
    if (!OPTION_IS_SOME(optional_component_size, stored_size)) {
        return OPTION_CREATE_NONE_STRUCT(component);
    } else if (component_storage_has(OPTION_GET(optional_component_storage, storage), entity_id)) {
        return OPTION_CREATE_SOME_STRUCT(
            component,
            component_storage_get(
                OPTION_GET(optional_component_storage, storage),
                entity_id,
                *OPTION_GET(optional_component_size, stored_size)
            )
        );
    } else {
        return OPTION_CREATE_NONE_STRUCT(component);
    }
}

void *world_components_get_component_unchecked(const world_components *wc, entity_id entity_id, component_id component_id) {
    return OPTION_GET(component, world_components_get_component(wc, entity_id, component_id));
}

optional_component world_components_remove_component(world_components *wc, entity_id entity_id, component_id component_id) {
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    if (!OPTION_IS_SOME(optional_component_size, stored_size)) {
        return OPTION_CREATE_NONE_STRUCT(component);
    } else {
        return component_storage_remove(
            OPTION_GET(optional_component_storage, world_components_get_component_storage(wc, component_id)),
            &wc->components_arena,
            entity_id,
            *OPTION_GET(optional_component_size, stored_size)
        );
    }
}

void *world_components_remove_component_unchecked(world_components *wc, entity_id entity_id, component_id component_id) {
    return OPTION_GET(component, world_components_remove_component(wc, entity_id, component_id));
}

#endif