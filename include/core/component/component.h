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
world_components_checksum world_components_checksum_hash(world_components_checksum current);
inline world_components_checksum world_components_checksum_add(world_components_checksum current, component_id component_id);

inline world_components_checksum world_components_checksum_remove(world_components_checksum current, component_id component_id) {
    return world_components_checksum_hash(current - (world_components_checksum)component_id);
}


typedef struct component_discard {
    void *handle;
    size_t size;
} component_discard;

typedef OPTION_STRUCT(size_t *, optional_component_size) optional_component_size;
typedef struct world_components {
    arena storages_arena;
    arena components_arena;
    paged_sparse_set component_storages;
    paged_sparse_set component_sizes;
    world_components_checksum checksum;
    component_discard discard;
} world_components;

world_components world_components_create(size_t component_type_capacity);

void world_components_free(world_components *wc);

size_t world_components_get_component_storage_count(const world_components *wc);

optional_component_size world_components_get_component_size(const world_components *wc, component_id component_id);
size_t world_components_get_component_size_unchecked(const world_components *wc, component_id component_id);

typedef OPTION_STRUCT(component_storage *, optional_component_storage) optional_component_storage;
optional_component_storage world_components_get_component_storage(const world_components *wc, component_id component_id);
component_storage *world_components_get_component_storage_unchecked(const world_components *wc, component_id component_id);

bool world_components_has_storage(const world_components *wc, component_id component_id);

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
);

optional_component world_components_set_component(
    world_components *wc,
    entity_id entity_id,
    component_id component_id,
    void *component,
    size_t size,
    component_storage_descriptor additional_storage_descriptor
);

void *world_components_set_component_unchecked(
    world_components *wc,
    entity_id entity_id,
    component_id component_id,
    void *component,
    size_t size,
    component_storage_descriptor additional_storage_descriptor
);

bool world_components_has_component(const world_components *wc, entity_id entity_id, component_id component_id);

optional_component world_components_get_component(const world_components *wc, entity_id entity_id, component_id component_id);

void *world_components_get_component_unchecked(const world_components *wc, entity_id entity_id, component_id component_id);

bool world_components_remove_component(
    world_components *wc,
    entity_id entity_id,
    component_id component_id,
    void *out_removed_component
);


typedef struct sized_component_storage {
    component_storage *storage;
    size_t component_size;
    component_id component_id;
} sized_component_storage;

typedef struct world_components_iterator {
    const world_components *const components;
    size_t storage_raw_index;
} world_components_iterator;

world_components_iterator world_components_iterator_create(const world_components *components);

bool world_components_iterator_done(const world_components_iterator *it);

size_t world_components_iterator_next(world_components_iterator *it);

sized_component_storage world_components_iterator_current(const world_components_iterator *it);


typedef struct world_components_entity_iterator {
    world_components_iterator it;
    const entity_id entity_id;
} world_components_entity_iterator;


world_components_entity_iterator world_components_entity_iterator_create(const world_components *components, entity_id entity_id);

bool world_components_entity_iterator_done(const world_components_entity_iterator *it);

size_t world_components_entity_iterator_next(world_components_entity_iterator *it);

sized_component_storage world_components_entity_iterator_current(const world_components_entity_iterator *it);

#endif