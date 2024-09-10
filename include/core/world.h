#ifndef WORLD_H
#define WORLD_H

#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include "../containers/arena.h"
#include "../containers/list.h"
#include "component/entity/entity.h"
#include "component/component.h"
#include "resource/resource.h"
#include "tag.h"
#include "relation.h"

typedef struct world {
    world_entities entities;
    world_components components;
    world_relations relations;
    world_resources resources;
} world;

world world_create(size_t entity_capacity, size_t component_type_capacity, size_t resource_capacity) {
    world w;
    w.entities = world_entities_create(entity_capacity);
    w.components = world_components_create(component_type_capacity);
    w.relations = world_relations_create(entity_capacity);

    const resource_default_size = sizeof(intptr_t) * 4;
    w.resources = world_resources_create(resource_capacity, resource_default_size);
    return w;
}

void world_free(world *w) {
    world_entities_free(&w->entities);
    world_components_free(&w->components);
    world_relations_free(&w->relations);
    world_resources_free(&w->resources);
    w->resources = (world_resources){0};
    w->relations = (world_relations){0};
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
        world_components_remove_component(&w->components, entity_id, key, &(optional_component){0});
        //component_storage_has()
    }
    return world_entities_remove_entity(&w->entities, entity_id);
}


void *world_set_component(world *w, entity_id entity_id, component_id component_id, void *component, size_t size) {  
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");    
    return world_components_set_component_unchecked(
        &w->components,
        entity_id,
        component_id,
        component,
        size,
        (component_storage_descriptor){
            .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = {0}
        }
    );
}
#define WORLD_SET_COMPONENT(type, world_ref, entity_id0, component_ref) \
    ((type *)world_set_component(world_ref, entity_id0, COMPONENT_ID(type), component_ref, sizeof(type)))

const bool world_remove_component(world *w, entity_id entity_id, component_id component_id, void *out_removed_component) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    return world_components_remove_component(&w->components, entity_id, component_id, out_removed_component);
}
#define WORLD_REMOVE_COMPONENT(type, world_ref, entity_id0, out_removed_component_ref) \
    (world_remove_component(world_ref, entity_id0, COMPONENT_ID(type), out_removed_component_ref))

void *world_get_component(const world *w, entity_id entity_id, component_id component_id) {
    return world_components_get_component_unchecked(&w->components, entity_id, component_id);
}
#define WORLD_GET_COMPONENT(type, world_ref, entity_id0) \
    ((type *)world_get_component(world_ref, entity_id0, COMPONENT_ID(type)))


tag_id world_add_tag(world *w, entity_id entity_id, tag_id tag_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    world_components_set_component(
        &w->components,
        entity_id,
        tag_id,
        NULL,
        0,
        (component_storage_descriptor){
            .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = {0}
        }
    );
    return tag_id;
}
#define WORLD_ADD_TAG(type, world_ref, entity_id0) \
    world_add_tag(world_ref, entity_id0, TAG_ID(type))

tag_id world_remove_tag(world *w, entity_id entity_id, tag_id tag_id) {
    assert((entity_id < world_entity_count(w)) && "Entity ID out of bounds");
    world_components_remove_component(&w->components, entity_id, tag_id, &(optional_component){0});
    return tag_id;
}
#define WORLD_REMOVE_TAG(type, world_ref, entity_id0) \
    world_remove_tag(world_ref, entity_id0, TAG_ID(type))


void *world_set_component_relation(world *w, entity_id id, component_id component_id, void *component, size_t size, tag_id tag_id) {
    assert((id < world_entity_count(w)) && "Entity ID out of bounds");
    entity_id extra_id = world_add_entity(w);
    entity_id associated_id = world_relations_increment_associated_id_or_set(
        &w->relations,
        id,
        (relation_target){tag_id},
        extra_id
    );
    if (associated_id != extra_id) {
        world_entities_remove_entity(&w->entities, extra_id);
    }

    return world_components_set_component_unchecked(
        &w->components,
        id,
        relation_id_create(relation_id_descriptor_create_with_tag_target(component_id, tag_id)),
        world_set_component(
            w,
            associated_id,
            component_id,
            component,
            size
        ),
        size,
        (component_storage_descriptor) {
            .capacity = 1,
            .indirect_component_id = OPTION_CREATE_SOME(indirect_component_id, component_id),
            .is_size_known = true,
        }
    );
}

void *world_get_component_relation(const world *w, entity_id id, component_id component_id, tag_id tag_id) {
    assert((id < world_entity_count(w)) && "Entity ID out of bounds");
    return world_get_component(w, id, relation_id_create(relation_id_descriptor_create_with_tag_target(component_id, tag_id)));
}

bool world_remove_component_relation(world *w, entity_id id, component_id component_id, void *out_removed_component, tag_id tag_id) {
    assert((id < world_entity_count(w)) && "Entity ID out of bounds");
    entity_id associated_id;
    if (!world_relations_remove_associated_id(&w->relations, id, (relation_target){tag_id}, &associated_id)) {
        return false;
    }

    return world_remove_component(
        w,
        id,
        relation_id_create(relation_id_descriptor_create_with_tag_target(component_id, tag_id)),
        out_removed_component
    ) && world_remove_component(
        w,
        associated_id,
        component_id,
        out_removed_component
    );
}
// TODO: tag-tag relations


resource_handle world_set_resource(world *w, resource_id id, void *resource, size_t size) {
    return world_resources_set_resource(&w->resources, id, resource, size);
}
#define WORLD_SET_RESOURCE(type, world_ref, resource_ref) \
    ((type *)world_set_resource(world_ref, RESOURCE_ID(type), resource_ref, sizeof(type)))

resource_handle world_get_resource(const world *w, resource_id id) {
    return world_resources_get_resource(&w->resources, id);
}
#define WORLD_GET_RESOURCE(type, world_ref) \
    ((type *)world_get_resource(world_ref, RESOURCE_ID(type)))

bool world_remove_resource(world *w, resource_id id) {
    return world_resources_remove_resource(&w->resources, id);
}
#define WORLD_REMOVE_RESOURCE(type, world_ref) \
    world_remove_resource(world_ref, RESOURCE_ID(type))

bool world_remove_resource_out(world *w, resource_id id, resource_handle out_resource, size_t size) {
    return world_resources_remove_resource_out(&w->resources, id, out_resource, size);
}
#define WORLD_REMOVE_RESOURCE_OUT(type, world_ref, out_resource_ref) \
    world_remove_resource_out(world_ref, RESOURCE_ID(type), out_resource_ref, sizeof(type))

#endif