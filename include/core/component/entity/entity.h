#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../containers/sparse_set.h"

#define COMPONENT_MASK_TYPE uint64_t
typedef COMPONENT_MASK_TYPE component_mask;

#define TAG_MASK_TYPE uint64_t
typedef TAG_MASK_TYPE tag_mask;

#define ENTITY_ID_TYPE uint64_t
typedef ENTITY_ID_TYPE entity_id;

typedef struct world_entities {
    arena entity_ids_arena;
    sparse_set entity_ids;
} world_entities;

world_entities world_entities_create(size_t entity_capacity) {
    world_entities we;
    we.entity_ids_arena = arena_create_with_capacity(sizeof(entity_id) * entity_capacity);
    we.entity_ids = sparse_set_create_of_integers_with_capacity(&we.entity_ids_arena, entity_capacity, sizeof(entity_id));
    return we;
}

void world_entities_free(world_entities *we) {
    arena_free(&we->entity_ids_arena);
    we->entity_ids = (sparse_set){0};
}

inline size_t world_entities_count(const world_entities *we) {
    return sparse_set_count_of_size(&we->entity_ids, sizeof(entity_id));
}

entity_id world_entities_add_entity(world_entities *we) {
    entity_id id = world_entities_count(we);
    return *SPARSE_SET_SET(
        entity_id,
        &we->entity_ids,
        &we->entity_ids_arena,
        id,
        &id
    );
}

entity_id world_entities_remove_entity(world_entities *we, entity_id id) {
    entity_id removed;
    SPARSE_SET_REMOVE_UNCHECKED(entity_id, &we->entity_ids, &we->entity_ids_arena, id, &removed);
    return removed;
}

#endif