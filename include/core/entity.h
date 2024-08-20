#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>

#define COMPONENT_MASK_TYPE uint32_t
typedef COMPONENT_MASK_TYPE component_mask;

#define TAG_MASK_TYPE uint32_t
typedef TAG_MASK_TYPE tag_mask;

typedef uint32_t entity_id;

typedef struct entity
{
    bool active;
    const entity_id id;
    component_mask components;
    tag_mask tags;
} entity;

entity entity_create(bool active, const entity_id id, component_mask components, tag_mask tags)
{
    return (entity){
        .active = active,
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

#define DEFAULT_ENTITY_CAPACITY 512

typedef struct linked_entity {
    entity *e;
    struct linked_entity *next;
} linked_entity;

linked_entity linked_entity_create(entity *e, linked_entity *next) {
    linked_entity le;
    le.e = e;
    le.next = next;
    return le;
}


typedef struct world_entities {
    linked_entity *last_unactive_entity;
    list entities;
} world_entities;

inline size_t world_entities_count(const world_entities *we) {
    return LIST_COUNT(entity, &we->entities);
}
inline entity *world_entities_get(const world_entities *we, entity_id entity_id) {
    return LIST_GET(entity, &we->entities, entity_id);
}

world_entities world_entities_create(arena *entities_arena) {
    world_entities we;
    we.last_unactive_entity = NULL;
    we.entities = LIST_CREATE(entity, entities_arena, DEFAULT_ENTITY_CAPACITY);
    return we;
}

entity *world_entities_add_enitity(world_entities *we, arena *entities_arena, component_mask components, tag_mask tags) {
    if (we->last_unactive_entity == NULL) {
        entity e = entity_create(true, world_entities_count(we), components, tags);
        return LIST_ADD(entity, &we->entities, entities_arena, &e);
    } else {
        linked_entity *le = we->last_unactive_entity;
        le->e->active = true;
        le->e->components = components;
        le->e->tags = tags;
        we->last_unactive_entity = le->next;
        return le->e;
    }
}

linked_entity *world_entities_remove_entity(world_entities *we, arena *entity_arena, entity_id entity_id) {
    assert((entity_id < world_entities_count(we)) && "Entity ID out of bounds");
    entity *e = world_entities_get(we, entity_id);
    e->active = false;
    e->components = 0;

    linked_entity *removed = (linked_entity *)arena_alloc(entity_arena, sizeof(linked_entity));
    *removed = linked_entity_create(e, we->last_unactive_entity);

    we->last_unactive_entity = removed;
    return removed;
}

#endif