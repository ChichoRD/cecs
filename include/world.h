#ifndef WORLD_H
#define WORLD_H

#include <assert.h>
#include <memory.h>
#include "entity.h"
#include "component.h"
#include "arena.h"
#include "list.h"
#include "resource.h"

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


#define COMPONENT_TYPE_COUNT (8 * sizeof(component_mask))
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
    assert((component_id < COMPONENT_TYPE_COUNT) && "Component ID out of bounds");
    if (wc->component_sizes[component_id] == -1) {
        if (wc->components[component_id].count != 0) {
            assert(0 && "unreachable: component_sizes and components are out of sync");
        }
        wc->component_sizes[component_id] = size;
        
        void *last_added = NULL;
        for (size_t i = 0; i < entity_count; i++) {
            last_added = list_add(&wc->components[component_id], components_arena, component, size);
        }
        return last_added;
    } else {
        assert((wc->component_sizes[component_id] == (ssize_t)size) && "Component size mismatch");
        return list_add(&wc->components[component_id], components_arena, component, size);
    }
}

void *world_components_set_component(world_components *wc, component_id component_id, entity_id entity_id, void *component, size_t size) {
    assert((component_id < COMPONENT_TYPE_COUNT) && "Component ID out of bounds");
    assert((entity_id < LIST_COUNT_OF_SIZE(wc->components[component_id], size)) && "Entity ID out of bounds");
    assert((wc->component_sizes[component_id] == (ssize_t)size) && "Component size mismatch");
    return list_set(&wc->components[component_id], entity_id, component, size);
}

void *world_components_get_component(const world_components *wc, component_id component_id, entity_id entity_id, size_t size) {
    assert((component_id < COMPONENT_TYPE_COUNT) && "Component ID out of bounds");
    assert((entity_id < LIST_COUNT_OF_SIZE(wc->components[component_id], size)) && "Entity ID out of bounds");
    assert((wc->component_sizes[component_id] == (ssize_t)size) && "Component size mismatch");
    return list_get(&wc->components[component_id], entity_id, size);
}
#define WORLD_COMPONENTS_GET(type, world_components0, entity_id0) (type *)world_components_get_component((world_components0), COMPONENT_ID(type), (entity_id0), sizeof(type))

void world_components_grow_all(world_components *wc, arena *components_arena, size_t entity_count) {
    for (size_t i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        ssize_t size = wc->component_sizes[i];
        if (size != -1) {
            if (LIST_COUNT_OF_SIZE(wc->components[i], size) <= 0) {
                assert(0 && "unreachable: component_sizes and components are out of sync");
            }     
            world_components_add_component(wc, components_arena, entity_count, i, list_get(&wc->components[i], 0, size), size);
        }
    }
}


#define DEFAULT_RESOURCE_CAPACITY 128

typedef ssize_t resource_offset;
typedef struct world_resources {
    list resources;
    list resource_offsets;
} world_resources;

world_resources world_resources_create(arena *resources_arena) {
    world_resources wr;
    wr.resources = list_create(resources_arena, (8 * DEFAULT_RESOURCE_CAPACITY));
    wr.resource_offsets = list_create(resources_arena, sizeof(resource_offset) * DEFAULT_RESOURCE_CAPACITY);
    return wr;
}

size_t world_resources_count(const world_resources *wr) {
    return LIST_COUNT_OF_SIZE(wr->resource_offsets, sizeof(resource_offset));
}

void *world_resources_add_resource(world_resources *wr, arena *resources_arena, resource_id id, void *resource, size_t size) {
    resource_offset offset = (id < world_resources_count(wr))
        ? *LIST_GET(resource_offset, &wr->resource_offsets, id)
        : wr->resources.count;
    if (offset >= 0) {
        assert((id == world_resources_count(wr)) && "Resource ID out of bounds");
        LIST_ADD(resource_offset, &wr->resource_offsets, resources_arena, offset);
        return list_add_range(&wr->resources, resources_arena, resource, size, sizeof(uint8_t));
    } else {
        if (id >= world_resources_count(wr)) {
            assert(0 && "unreachable: resource_offsets and resources are out of sync");
        }
        resource_offset removed_offset = -offset - 1;
        LIST_SET(resource_offset, &wr->resource_offsets, id, removed_offset);
        return (uint8_t *)wr->resources.elements + removed_offset;
    }
}
#define WORLD_RESOURCES_ADD_RESOURCE(type, world_resources0, resources_arena0, resource) (type *)world_resources_add_resource(&(world_resources0), &(resources_arena0), RESOURCE_ID(type), &(resource), sizeof(type))

void *world_resources_get_resource(const world_resources *wr, resource_id id, size_t size) {
    assert((id < world_resources_count(wr)) && "Resource ID out of bounds");
    resource_offset offset = *LIST_GET(resource_offset, &wr->resource_offsets, id);
    assert((offset >= 0) && "Resource has been removed");

    void *resource = (uint8_t *)wr->resources.elements + offset;
    assert(((uint8_t *)resource + size <= (uint8_t *)wr->resources.elements + wr->resources.count) && "Resource out of bounds");
    return resource;
}
#define WORLD_RESOURCES_GET_RESOURCE(type, world_resources0) (type *)world_resources_get_resource(&(world_resources0), RESOURCE_ID(type), sizeof(type))

void *world_resources_remove_resource(world_resources *wr, resource_id id, size_t size) {
    void *resource = world_resources_get_resource(wr, id, size);

    resource_offset removed_offset = (uint8_t *)wr->resources.elements - (uint8_t *)resource - 1;
    LIST_SET(resource_offset, &wr->resource_offsets, id, removed_offset);
    return resource;
}
#define WORLD_RESOURCES_REMOVE_RESOURCE(type, world_resources0) (type *)world_resources_remove_resource(&(world_resources0), RESOURCE_ID(type), sizeof(type))

typedef struct world {
    arena entities_arena;
    world_entities entities;

    arena components_arena;
    world_components components;

    arena resources_arena;
    world_resources resources;
} world;

world world_create(void) {
    world w;
    w.entities_arena = arena_create();
    w.entities = world_entities_create(&w.entities_arena);

    w.components_arena = arena_create();
    w.components = world_components_create(&w.components_arena);

    w.resources_arena = arena_create();
    w.resources = world_resources_create(&w.resources_arena);
    return w;
}

void world_free(world *w) {
    arena_free(&w->entities_arena);
    arena_free(&w->components_arena);
    arena_free(&w->resources_arena);
}

size_t world_entity_count(const world *w) {
    return WORLD_ENTITIES_COUNT(w->entities);
}

entity_id world_add_entity(world *w) {
    entity_id id = world_entities_add_enitity(&w->entities, &w->entities_arena, 0)->id;
    world_components_grow_all(&w->components, &w->components_arena, WORLD_ENTITIES_COUNT(w->entities));
    return id;
}

entity *world_get_entity(const world *w, entity_id entity_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    return WORLD_ENTITIES_GET(w->entities, entity_id);
}

entity_id world_remove_entity(world *w, entity_id entity_id) {
    return world_entities_remove_entity(&w->entities, &w->entities_arena, entity_id)->e->id;
}

void *world_set_or_add_component(world *w, entity_id entity_id, component_id component_id, void *component, size_t size) {  
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    uint32_t mask = 1 << component_id;
    e->mask |= mask;
    
    if (entity_id < LIST_COUNT_OF_SIZE(w->components.components[component_id], size)) {
        return world_components_set_component(&w->components, component_id, entity_id, component, size);
    } else {
        return world_components_add_component(&w->components, &w->components_arena, WORLD_ENTITIES_COUNT(w->entities), component_id, component, size);
    }
}
#define WORLD_SET_OR_ADD_COMPONENT(type, world0, entity_id0, component) ((type *)world_set_or_add_component(&(world0), (entity_id0), COMPONENT_ID(type), &(component), sizeof(type)))

void *world_remove_component(world *w, entity_id entity_id, component_id component_id, size_t size) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    uint32_t mask = 1 << component_id;
    e->mask &= ~mask;
    return world_components_get_component(&w->components, component_id, entity_id, size);
}
#define WORLD_REMOVE_COMPONENT(type, world0, entity_id0) (*(type *)world_remove_component(&(world0), (entity_id0), COMPONENT_ID(type), sizeof(type)))

void *world_get_component(const world *w, component_id component_id, entity_id entity_id, size_t size) {
    return world_components_get_component(&w->components, component_id, entity_id, size);
}
#define WORLD_GET_COMPONENT(type, world0, entity_id0) ((type *)world_get_component(&(world0), COMPONENT_ID(type), (entity_id0), sizeof(type)))

void *world_add_resource(world *w, resource_id id, void *resource, size_t size) {
    return world_resources_add_resource(&w->resources, &w->resources_arena, id, resource, size);
}
#define WORLD_ADD_RESOURCE(type, world0, resource0) ((type *)world_add_resource(&(world0), RESOURCE_ID(type), &(resource0), sizeof(type)))

void *world_get_resource(const world *w, resource_id id, size_t size) {
    return world_resources_get_resource(&w->resources, id, size);
}
#define WORLD_GET_RESOURCE(type, world0) ((type *)world_get_resource(&(world0), RESOURCE_ID(type), sizeof(type)))

void *world_remove_resource(world *w, resource_id id, size_t size) {
    return world_resources_remove_resource(&w->resources, id, size);
}
#define WORLD_REMOVE_RESOURCE(type, world0) (*(type *)world_remove_resource(&(world0), RESOURCE_ID(type), sizeof(type)))

#endif