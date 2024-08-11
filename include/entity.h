#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>

#define COMPONENT_MASK_TYPE uint32_t
typedef COMPONENT_MASK_TYPE component_mask;
typedef uint32_t entity_id;

typedef struct entity
{
    bool active;
    const entity_id id;
    component_mask mask;
} entity;

entity entity_create(bool active, const entity_id id, component_mask mask)
{
    return (entity){
        .active = active,
        .id = id,
        .mask = mask,
    };
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

#define WORLD_ENTITIES_COUNT(world_entities0) LIST_COUNT(entity, (world_entities0).entities)
#define WORLD_ENTITIES_GET(world_entities0, entity_id0) LIST_GET(entity, &((world_entities0).entities), entity_id0)

world_entities world_entities_create(arena *entities_arena) {
    world_entities we;
    we.last_unactive_entity = NULL;
    we.entities = LIST_CREATE(entity, entities_arena, DEFAULT_ENTITY_CAPACITY);
    return we;
}

entity *world_entities_add_enitity(world_entities *we, arena *entities_arena, component_mask mask) {
    if (we->last_unactive_entity == NULL) {
        entity e = entity_create(true, WORLD_ENTITIES_COUNT(*we), mask);
        return &LIST_ADD(entity, &we->entities, entities_arena, e);
    } else {
        linked_entity *le = we->last_unactive_entity;
        le->e->active = true;
        le->e->mask = mask;
        we->last_unactive_entity = le->next;
        return le->e;
    }
}

linked_entity *world_entities_remove_entity(world_entities *we, arena *entity_arena, entity_id entity_id) {
    assert((entity_id < WORLD_ENTITIES_COUNT(*we)) && "Entity ID out of bounds");
    entity *e = WORLD_ENTITIES_GET(*we, entity_id);
    e->active = false;
    e->mask = 0;

    linked_entity *removed = (linked_entity *)arena_alloc(entity_arena, sizeof(linked_entity));
    *removed = linked_entity_create(e, we->last_unactive_entity);

    we->last_unactive_entity = removed;
    return removed;
}

#endif