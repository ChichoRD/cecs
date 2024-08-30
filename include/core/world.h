#ifndef WORLD_H
#define WORLD_H

#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include "entity.h"
#include "component/component.h"
#include "tag.h"
#include "../containers/arena.h"
#include "../containers/list.h"
#include "resource.h"

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
    w.components = world_components_create(&w.components_arena, DEFAULT_ENTITY_CAPACITY);

    w.resources_arena = arena_create();
    w.resources = world_resources_create(&w.resources_arena);
    return w;
}

void world_free(world *w) {
    arena_free(&w->entities_arena);
    arena_free(&w->components_arena);
    arena_free(&w->resources_arena);
}

inline size_t world_entity_count(const world *w) {
    return world_entities_count(&w->entities);
}

entity_id world_add_entity(world *w) {
    entity_id id = world_entities_add_enitity(&w->entities, &w->entities_arena, 0, 0)->id;
    world_components_grow_all(&w->components, &w->components_arena, world_entity_count(w));
    return id;
}

entity *world_get_entity(const world *w, entity_id entity_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    return world_entities_get(&w->entities, entity_id);
}

entity_id world_remove_entity(world *w, entity_id entity_id) {
    return world_entities_remove_entity(&w->entities, &w->entities_arena, entity_id)->e->id;
}

void *world_set_or_add_component(world *w, entity_id entity_id, component_id component_id, void *component, size_t size) {  
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    uint32_t mask = 1 << component_id;
    e->components |= mask;
    
    if (entity_id < list_count_of_size(&w->components.components[component_id], size)) {
        return world_components_set_component(&w->components, component_id, entity_id, component, size);
    } else {
        return world_components_add_component(&w->components, &w->components_arena, world_entities_count(&w->entities), component_id, component, size);
    }
}
#define WORLD_SET_OR_ADD_COMPONENT(type, world_ref, entity_id0, component_ref) \
    ((type *)world_set_or_add_component(world_ref, entity_id0, COMPONENT_ID(type), component_ref, sizeof(type)))

const void *world_remove_component(world *w, entity_id entity_id, component_id component_id, size_t size) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    uint32_t mask = 1 << component_id;
    e->components &= ~mask;
    return world_components_get_component(&w->components, component_id, entity_id, size);
}
#define WORLD_REMOVE_COMPONENT(type, world_ref, entity_id0) \
    (*(type *)world_remove_component(world_ref, entity_id0, COMPONENT_ID(type), sizeof(type)))

void *world_get_component(const world *w, component_id component_id, entity_id entity_id, size_t size) {
    return world_components_get_component(&w->components, component_id, entity_id, size);
}
#define WORLD_GET_COMPONENT(type, world_ref, entity_id0) \
    ((type *)world_get_component(world_ref, COMPONENT_ID(type), entity_id0, sizeof(type)))

tag_mask world_add_tag(world *w, entity_id entity_id, tag_id tag_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    tag_mask mask = 1 << tag_id;
    e->tags |= mask;
    return e->tags;
}
#define WORLD_ADD_TAG(type, world_ref, entity_id0) \
    world_add_tag(world_ref, entity_id0, TAG_ID(type))

tag_mask world_remove_tag(world *w, entity_id entity_id, tag_id tag_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    tag_mask mask = 1 << tag_id;
    e->tags &= ~mask;
    return e->tags;
}
#define WORLD_REMOVE_TAG(type, world_ref, entity_id0) \
    world_remove_tag(world_ref, entity_id0, TAG_ID(type))

tag_mask world_set_tags(world *w, entity_id entity_id, tag_mask tags) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    e->tags = tags;
    return e->tags;
}
#define WORLD_SET_TAGS(world_ref, entity_id0, ...) \
    world_set_tags(world_ref, entity_id0, TAGS_MASK(__VA_ARGS__))

tag_mask world_add_tags(world *w, entity_id entity_id, tag_mask tags) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    e->tags |= tags;
    return e->tags;
}
#define WORLD_ADD_TAGS(world_ref, entity_id0, ...) \
    world_add_tags(world_ref, entity_id0, TAGS_MASK(__VA_ARGS__))

tag_mask world_remove_tags(world *w, entity_id entity_id, tag_mask tags) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    entity *e = world_get_entity(w, entity_id);
    e->tags &= ~tags;
    return e->tags;
}
#define WORLD_REMOVE_TAGS(world_ref, entity_id0, ...) \
    world_remove_tags(world_ref, entity_id0, TAGS_MASK(__VA_ARGS__))

void *world_set_or_add_resource(world *w, resource_id id, void *resource, size_t size) {
    if (id < world_resources_count(&w->resources)) {
        return world_resources_set_resource(&w->resources, id, resource, size);
    } else {
        return world_resources_add_resource(&w->resources, &w->resources_arena, id, resource, size);
    }
}
#define WORLD_SET_OR_ADD_RESOURCE(type, world_ref, resource_ref) \
    ((type *)world_set_or_add_resource(world_ref, RESOURCE_ID(type), resource_ref, sizeof(type)))

void *world_get_resource(const world *w, resource_id id, size_t size) {
    return world_resources_get_resource(&w->resources, id, size);
}
#define WORLD_GET_RESOURCE(type, world_ref) \
    ((type *)world_get_resource(world_ref, RESOURCE_ID(type), sizeof(type)))

const void *world_remove_resource(world *w, resource_id id, size_t size) {
    return world_resources_remove_resource(&w->resources, id, size);
}
#define WORLD_REMOVE_RESOURCE(type, world_ref) \
    (*(type *)world_remove_resource(world_ref, RESOURCE_ID(type), sizeof(type)))

#endif