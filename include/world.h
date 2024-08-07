#ifndef WORLD_H
#define WORLD_H

#include <assert.h>
#include <memory.h>
#include "../include/entity.h"
#include "../include/component.h"
#include "../include/arena.h"
#include "../include/list.h"

#define DEFAULT_ENTITY_CAPACITY 512

typedef struct world_entities {
    list entities;
} world_entities;

world_entities world_entities_create(arena *entities_arena) {
    world_entities we;
    we.entities = LIST_CREATE(entity, entities_arena, DEFAULT_ENTITY_CAPACITY);
    return we;
}

entity *world_entities_add_enitity(world_entities *we, arena *entities_arena, COMPONENT_MASK mask) {
    entity e = entity_create(true, LIST_COUNT(entity, we->entities), mask);
    return &LIST_ADD(entity, &we->entities, entities_arena, e);
}


#define COMPONENT_TYPE_COUNT (8 * sizeof(COMPONENT_MASK))
#define DEFAULT_COMPONENT_CAPACITY (8 * DEFAULT_ENTITY_CAPACITY)

typedef struct world {
    arena entities_arena;
    world_entities entities;

    arena components_arena;
    list components[COMPONENT_TYPE_COUNT];
} world;

#define WORLD_FOREACH_COMPONENT_LIST(world0, component_id0, component_list, do) \
    list component_list; \
    for (size_t component_id0 = 0; component_id0 < COMPONENT_TYPE_COUNT; component_id0++) \
    { \
        component_list = world0.components[component_id0]; \
        do \
    } \


world world_create(void) {
    world w;
    w.entities_arena = arena_create();
    w.entities = world_entities_create(&w.entities_arena);

    w.components_arena = arena_create();
    for (size_t i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        w.components[i] = list_create(&w.components_arena, DEFAULT_COMPONENT_CAPACITY);
    }
    return w;
}

void world_free(world *w) {
    arena_free(&w->entities_arena);
    arena_free(&w->components_arena);
}

entity_id world_add_entity(world *w, COMPONENT_MASK mask) {
    return world_entities_add_enitity(&w->entities, &w->entities_arena, mask)->id;
}

void *world_add_component(world *w, entity_id entity_id, component_id component_id, void *component, size_t size) {  
    printf("entity_id: %d\n", entity_id);
    assert(entity_id < LIST_COUNT(entity, w->entities.entities));
    entity *e = &LIST_GET(entity, &w->entities.entities, entity_id);
    uint32_t mask = 1 << component_id;
    e->mask |= mask;
    
    assert(component_id < COMPONENT_TYPE_COUNT);
    if (size != 0)
        assert(w->components[component_id].count % size == 0);

    if (entity_id < LIST_COUNT_OF_SIZE(w->components[component_id], size)) {
        return list_set(&w->components[component_id], entity_id, component, size);
    } else {
        return list_add(&w->components[component_id], &w->components_arena, component, size);
    }
}
#define WORLD_ADD_COMPONENT(type, world0, entity_id0, component_id0, component) *(type *)world_add_component((world0), (entity_id0), (component_id0), &(component), sizeof(type))

#endif