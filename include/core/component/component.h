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

typedef uint32_t world_components_checksum;
world_components_checksum world_components_checksum_hash(world_components_checksum current) {
    const uint32_t fnv32_prime = 0x01000193;
    const uint32_t fnv32_basis = 0x811C9DC5;
    return (fnv32_basis ^ current) * fnv32_prime;
}
inline world_components_checksum world_components_checksum_add(world_components_checksum current, component_id component_id) {
    return world_components_checksum_hash(current + component_id);
}

inline world_components_checksum world_components_checksum_remove(world_components_checksum current, component_id component_id) {
    return world_components_checksum_hash(current - component_id);
}


typedef struct world_components {
    arena storages_arena;
    arena components_arena;
    sparse_set component_storages;
    sparse_set component_sizes;
    world_components_checksum checksum;
} world_components;

world_components world_components_create(size_t component_type_capacity) {
    return (world_components) {
        .storages_arena = arena_create_with_capacity(sizeof(component_storage) * component_type_capacity),
        .components_arena = arena_create(),
        .component_storages = sparse_set_create(),
        .component_sizes = sparse_set_create(),
        .checksum = 0
    };
}

void world_components_free(world_components *wc) {
    arena_free(&wc->components_arena);
    arena_free(&wc->storages_arena);
    wc->component_storages = (sparse_set){0};
    wc->component_sizes = (sparse_set){0};
}

size_t world_components_get_component_storage_count(const world_components *wc) {
    return sparse_set_count_of_size(&wc->component_storages, sizeof(component_storage));
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
component_storage *world_components_get_component_storage_unchecked(const world_components *wc, component_id component_id) {
    return OPTION_GET(optional_component_storage, world_components_get_component_storage(wc, component_id));
}

bool world_components_has_storage(const world_components *wc, component_id component_id) {
    return sparse_set_contains(&wc->component_storages, component_id);
}

typedef component_id indirect_component_id;
typedef struct component_storage_descriptor {
    bool is_size_known;
    size_t capacity;
    OPTION_STRUCT(component_id, indirect_component_id) indirect_component_id;
} component_storage_descriptor;

component_storage component_storage_descriptor_build(
    component_storage_descriptor descriptor,
    world_components *wc,
    size_t component_size
) {
    if (component_size == 0 && descriptor.is_size_known) {
        return component_storage_create_unit(&wc->components_arena);
    }

    if (!descriptor.is_size_known)
        assert(component_size == 0 && "component_size must be zero if is_size_known is false");

    if (OPTION_IS_SOME(indirect_component_id, descriptor.indirect_component_id)) {
        component_id other_id = OPTION_GET(indirect_component_id, descriptor.indirect_component_id);
        if (world_components_has_storage(wc, other_id)) {
            return component_storage_create_indirect(
                &wc->components_arena,
                world_components_get_component_storage_unchecked(wc, other_id)
            );
        } else {
            component_storage other_storage = component_storage_descriptor_build((component_storage_descriptor){
                .is_size_known = false,
                .capacity = descriptor.capacity,
                .indirect_component_id = OPTION_CREATE_NONE(indirect_component_id)
            }, wc, &wc->components_arena, 0);
            return component_storage_create_indirect(
                &wc->components_arena,
                SPARSE_SET_SET(component_storage, &wc->component_storages, &wc->storages_arena, other_id, &other_storage)
            );
        }
    } else {
        return component_storage_create_sparse(&wc->components_arena, descriptor.capacity, component_size);
    }
}

optional_component world_components_set_component(
    world_components *wc,
    entity_id entity_id,
    component_id component_id,
    void *component,
    size_t size,
    component_storage_descriptor additional_storage_descriptor
) {
    wc->checksum = world_components_checksum_add(wc->checksum, component_id);
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
        component_storage storage = component_storage_descriptor_build(additional_storage_descriptor, wc, size);
        return component_storage_set(
            SPARSE_SET_SET(component_storage, &wc->component_storages, &wc->storages_arena, component_id, &storage),
            &wc->components_arena,
            entity_id,
            component,
            size
        );
    }
}

void *world_components_set_component_unchecked(
    world_components *wc,
    entity_id entity_id,
    component_id component_id,
    void *component,
    size_t size,
    component_storage_descriptor additional_storage_descriptor
) {
    return OPTION_GET(optional_component, world_components_set_component(wc, entity_id, component_id, component, size, additional_storage_descriptor));
}

optional_component world_components_get_component(const world_components *wc, entity_id entity_id, component_id component_id) {
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    optional_component_storage storage = world_components_get_component_storage(wc, component_id);
    if (!OPTION_IS_SOME(optional_component_size, stored_size)) {
        return OPTION_CREATE_NONE_STRUCT(optional_component);
    } else if (component_storage_has(OPTION_GET(optional_component_storage, storage), entity_id)) {
        return component_storage_get(
            OPTION_GET(optional_component_storage, storage),
            entity_id,
            *OPTION_GET(optional_component_size, stored_size)
        );
    } else {
        return OPTION_CREATE_NONE_STRUCT(optional_component);
    }
}

void *world_components_get_component_unchecked(const world_components *wc, entity_id entity_id, component_id component_id) {
    return OPTION_GET(optional_component, world_components_get_component(wc, entity_id, component_id));
}

bool world_components_remove_component(
    world_components *wc,
    entity_id entity_id,
    component_id component_id,
    void *out_removed_component
) {
    wc->checksum = world_components_checksum_remove(wc->checksum, component_id);
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    if (OPTION_IS_NONE(optional_component_size, stored_size)) {
        assert(out_removed_component == NULL && "out_removed_component must be NULL if component size is unknown");
        return false;
    } else {
        return component_storage_remove(
            OPTION_GET(optional_component_storage, world_components_get_component_storage(wc, component_id)),
            &wc->components_arena,
            entity_id,
            out_removed_component,
            *OPTION_GET(optional_component_size, stored_size)
        );
    }
}

#endif