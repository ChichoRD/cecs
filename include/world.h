#ifndef WORLD_H
#define WORLD_H

#include <assert.h>
#include <memory.h>
#include "../include/entity.h"
#include "../include/component.h"
#include "../include/arena.h"
#include "../include/list.h"

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

world_entities world_entities_create(arena *entities_arena) {
    world_entities we;
    we.last_unactive_entity = NULL;
    we.entities = LIST_CREATE(entity, entities_arena, DEFAULT_ENTITY_CAPACITY);
    return we;
}

entity *world_entities_add_enitity(world_entities *we, arena *entities_arena, COMPONENT_MASK mask) {
    if (we->last_unactive_entity == NULL) {
        entity e = entity_create(true, LIST_COUNT(entity, we->entities), mask);
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
    assert(entity_id < LIST_COUNT(entity, we->entities));
    entity *e = &LIST_GET(entity, &we->entities, entity_id);
    e->active = false;
    e->mask = 0;

    linked_entity *removed = (linked_entity *)arena_alloc(entity_arena, sizeof(linked_entity));
    *removed = linked_entity_create(e, we->last_unactive_entity);

    we->last_unactive_entity = removed;
    return removed;
}


#define COMPONENT_TYPE_COUNT (8 * sizeof(COMPONENT_MASK))
#define DEFAULT_COMPONENT_CAPACITY (8 * DEFAULT_ENTITY_CAPACITY)

typedef int32_t ssize_t;

typedef struct world_components {
    list components[COMPONENT_TYPE_COUNT];
    ssize_t component_sizes[COMPONENT_TYPE_COUNT];
} world_components;

world_components world_components_create(arena *components_arena) {
    world_components wc;
    for (size_t i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        wc.components[i] = list_create(components_arena, DEFAULT_COMPONENT_CAPACITY);
        wc.component_sizes[i] = -1;
    }
    return wc;
}

void *world_components_add_component(world_components *wc, arena *components_arena, size_t entity_count, component_id component_id, void *component, size_t size) {
    assert(component_id < COMPONENT_TYPE_COUNT);
    if (wc->component_sizes[component_id] == -1) {
        assert(wc->components[component_id].count == 0);
        wc->component_sizes[component_id] = size;
        
        void *last_added = NULL;
        for (size_t i = 0; i < entity_count; i++) {
            last_added = list_add(&wc->components[component_id], components_arena, component, size);
        }
        return last_added;
    } else {
        assert(wc->component_sizes[component_id] == (ssize_t)size);
        return list_add(&wc->components[component_id], components_arena, component, size);
    }
}

void *world_components_set_component(world_components *wc, component_id component_id, entity_id entity_id, void *component, size_t size) {
    assert(component_id < COMPONENT_TYPE_COUNT);
    assert(entity_id < LIST_COUNT_OF_SIZE(wc->components[component_id], size));
    assert(wc->component_sizes[component_id] == (ssize_t)size);
    return list_set(&wc->components[component_id], entity_id, component, size);
}

void world_components_grow_all(world_components *wc, arena *components_arena, size_t entity_count) {
    for (size_t i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        ssize_t size = wc->component_sizes[i];
        if (size != -1) {
            assert(LIST_COUNT_OF_SIZE(wc->components[i], size) > 0);
            world_components_add_component(wc, components_arena, entity_count, i, list_get(&wc->components[i], 0, size), size);
        }
    }
}


typedef struct world {
    arena entities_arena;
    world_entities entities;

    arena components_arena;
    world_components components;
} world;

world world_create(void) {
    world w;
    w.entities_arena = arena_create();
    block b = block_create(4);
    arena_prepend(&w.entities_arena, &b);
    w.entities = world_entities_create(&w.entities_arena);

    w.components_arena = arena_create();
    w.components = world_components_create(&w.components_arena);
    return w;
}

void world_free(world *w) {
    arena_free(&w->entities_arena);
    arena_free(&w->components_arena);
}

entity_id world_add_entity(world *w, COMPONENT_MASK mask) {
    entity_id id = world_entities_add_enitity(&w->entities, &w->entities_arena, mask)->id;
    world_components_grow_all(&w->components, &w->components_arena, LIST_COUNT(entity, w->entities.entities));
    return id;
}

void *world_set_or_add_component(world *w, entity_id entity_id, component_id component_id, void *component, size_t size) {  
    assert(entity_id < LIST_COUNT(entity, w->entities.entities));
    entity *e = &LIST_GET(entity, &w->entities.entities, entity_id);
    uint32_t mask = 1 << component_id;
    e->mask |= mask;
    
    if (entity_id < LIST_COUNT_OF_SIZE(w->components.components[component_id], size)) {
        return world_components_set_component(&w->components, component_id, entity_id, component, size);
    } else {
        return world_components_add_component(&w->components, &w->components_arena, LIST_COUNT(entity, w->entities.entities), component_id, component, size);
    }
}
#define WORLD_SET_OR_ADD_COMPONENT(type, world0, entity_id0, component) *(type *)world_set_or_add_component((world0), (entity_id0), COMPONENT_ID(type), &(component), sizeof(type))

entity_id world_remove_entity(world *w, entity_id entity_id) {
    return world_entities_remove_entity(&w->entities, &w->entities_arena, entity_id)->e->id;
}

#endif