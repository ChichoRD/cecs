#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../containers/sparse_set.h"

#define COMPONENT_MASK_TYPE uint64_t
typedef COMPONENT_MASK_TYPE component_mask;

#define TAG_MASK_TYPE uint64_t
typedef TAG_MASK_TYPE tag_mask;

typedef uint64_t entity_id;

typedef struct entity
{
    const entity_id id;
    component_mask components;
    tag_mask tags;
} entity;

entity entity_create(const entity_id id, component_mask components, tag_mask tags)
{
    return (entity){
        .id = id,
        .components = components,
        .tags = tags
    };
}

inline bool entity_has_components(const entity *e, const component_mask components) {
    return (e->components & components) == components;
}

inline bool entity_has_tags(const entity *e, const tag_mask tags) {
    return (e->tags & tags) == tags;
}


typedef struct world_entities {
    arena entities_arena;
    sparse_set entities;
} world_entities;

world_entities world_entities_create(size_t entity_capacity) {
    world_entities we;
    we.entities_arena = arena_create_with_capacity(sizeof(entity) * entity_capacity);
    we.entities = sparse_set_create(&we.entities_arena, entity_capacity, sizeof(entity));
    return we;
}

void world_entities_free(world_entities *we) {
    arena_free(&we->entities_arena);
    we->entities = (sparse_set){0};
}

inline size_t world_entities_count(const world_entities *we) {
    return sparse_set_count(&we->entities);
}

inline entity *world_entities_get(const world_entities *we, entity_id id) {
    return SPARSE_SET_GET_UNCHECKED(entity, &we->entities, id);
}

entity *world_entities_add_entity(world_entities *we, component_mask components, tag_mask tags) {
    entity_id id = world_entities_count(we);
    entity e = entity_create(id, components, tags);
    return SPARSE_SET_SET(
        entity,
        &we->entities,
        &we->entities_arena,
        id,
        &e
    );
}

entity world_entities_remove_entity(world_entities *we, entity_id id) {
    entity removed;
    SPARSE_SET_REMOVE_UNCHECKED(entity, &we->entities, &we->entities_arena, id, &removed);
    return removed;
}

#endif