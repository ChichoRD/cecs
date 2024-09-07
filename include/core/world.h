#ifndef WORLD_H
#define WORLD_H

#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include "component/entity/entity.h"
#include "component/component.h"
#include "tag.h"
#include "../containers/arena.h"
#include "../containers/list.h"

typedef struct world {
    world_entities entities;
    world_components components;

    // world_resources resources;
} world;

world world_create(size_t entity_capacity, size_t component_type_capacity) {
    world w;
    w.entities = world_entities_create(entity_capacity);
    w.components = world_components_create(component_type_capacity);

    // w.resources = world_resources_create(&w.resources_arena);
    return w;
}

void world_free(world *w) {
    world_entities_free(&w->entities);
    world_components_free(&w->components);
    w->components = (world_components){0};
    w->entities = (world_entities){0};
}

inline size_t world_entity_count(const world *w) {
    return world_entities_count(&w->entities);
}

entity_id world_add_entity(world *w) {
    return world_entities_add_entity(&w->entities, 0, 0);
}

entity_id world_remove_entity(world *w, entity_id entity_id) {
    for (size_t i = 0; i < world_components_get_component_storage_count(&w->components); i++) {
        component_id key = sparse_set_keys(&w->components.component_storages)[i];
        world_components_remove_component(&w->components, entity_id, key);
    }
    return world_entities_remove_entity(&w->entities, entity_id);
}


void *world_set_component(world *w, entity_id entity_id, component_id component_id, void *component, size_t size) {  
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");    
    return world_components_set_component_unchecked(&w->components, entity_id, component_id, component, size);
}
#define WORLD_SET_COMPONENT(type, world_ref, entity_id0, component_ref) \
    ((type *)world_set_component(world_ref, entity_id0, COMPONENT_ID(type), component_ref, sizeof(type)))

const void *world_remove_component(world *w, entity_id entity_id, component_id component_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    return world_components_remove_component_unchecked(&w->components, entity_id, component_id);
}
#define WORLD_REMOVE_COMPONENT(type, world_ref, entity_id0) \
    (*(type *)world_remove_component(world_ref, entity_id0, COMPONENT_ID(type)))

void *world_get_component(const world *w, entity_id entity_id, component_id component_id) {
    return world_components_get_component_unchecked(&w->components, entity_id, component_id);
}
#define WORLD_GET_COMPONENT(type, world_ref, entity_id0) \
    ((type *)world_get_component(world_ref, entity_id0, COMPONENT_ID(type)))


tag_id world_add_tag(world *w, entity_id entity_id, tag_id tag_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    world_components_set_component(&w->components, entity_id, tag_id, NULL, 0);
    return tag_id;
}
#define WORLD_ADD_TAG(type, world_ref, entity_id0) \
    world_add_tag(world_ref, entity_id0, TAG_ID(type))

tag_id world_remove_tag(world *w, entity_id entity_id, tag_id tag_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    world_components_remove_component(&w->components, entity_id, tag_id);
    return tag_id;
}
#define WORLD_REMOVE_TAG(type, world_ref, entity_id0) \
    world_remove_tag(world_ref, entity_id0, TAG_ID(type))

#endif