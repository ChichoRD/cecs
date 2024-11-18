#ifndef WORLD_H
#define WORLD_H

#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include "../containers/arena.h"
#include "../containers/list.h"
#include "component/entity/entity.h"
#include "component/entity/tag.h"
#include "component/entity/game_component.h"
#include "component/component.h"
#include "resource/resource.h"
#include "relation.h"


#define WORLD_UNIQUE_RELATION_COMPONENTS true
#define WORLD_FLAG_ALL_ENTITIES true


typedef struct world {
    world_entities entities;
    world_components components;
    world_relations relations;
    world_resources resources;
} world;

world world_create(size_t entity_capacity, size_t component_type_capacity, size_t resource_capacity);

void world_free(world *w);


inline size_t world_entity_count(const world* w) {
    return world_entities_count(&w->entities);
}


void *world_get_component(const world *w, entity_id entity_id, component_id component_id);
#define WORLD_GET_COMPONENT(type, world_ref, entity_id0) \
    ((type *)world_get_component(world_ref, entity_id0, COMPONENT_ID(type)))

bool world_try_get_component(const world *w, entity_id entity_id, component_id component_id, void **out_component);
#define WORLD_TRY_GET_COMPONENT(type, world_ref, entity_id0, out_component_ref) \
    (world_try_get_component(world_ref, entity_id0, COMPONENT_ID(type), out_component_ref))


entity_flags world_get_entity_flags(const world *w, entity_id entity_id);

void *world_set_component(world *w, entity_id id, component_id component_id, void *component, size_t size);
#define WORLD_SET_COMPONENT(type, world_ref, entity_id0, component_ref) \
    ((type *)world_set_component(world_ref, entity_id0, COMPONENT_ID(type), component_ref, sizeof(type)))


const bool world_remove_component(world *w, entity_id id, component_id component_id, void *out_removed_component);
#define WORLD_REMOVE_COMPONENT(type, world_ref, entity_id0, out_removed_component_ref) \
    (world_remove_component(world_ref, entity_id0, COMPONENT_ID(type), out_removed_component_ref))

inline entity_flags *world_set_entity_flags(world *w, entity_id id, entity_flags flags) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return WORLD_SET_COMPONENT(entity_flags, w, id, &flags);
}

entity_flags *world_get_or_set_default_entity_flags(world *w, entity_id id);


bool world_has_tag(const world *w, entity_id id, tag_id tag_id);
#define WORLD_HAS_TAG(type, world_ref, entity_id0) \
    world_has_tag(world_ref, entity_id0, TAG_ID(type))

tag_id world_add_tag(world *w, entity_id id, tag_id tag_id);
#define WORLD_ADD_TAG(type, world_ref, entity_id0) \
    world_add_tag(world_ref, entity_id0, TAG_ID(type))

tag_id world_remove_tag(world *w, entity_id id, tag_id tag_id);
#define WORLD_REMOVE_TAG(type, world_ref, entity_id0) \
    world_remove_tag(world_ref, entity_id0, TAG_ID(type))


entity_id world_add_entity(world *w) {
    entity_id e = world_entities_add_entity(&w->entities);
#if WORLD_FLAG_ALL_ENTITIES
    world_set_entity_flags(w, e, entity_flags_default());
#endif
    return e;
}

entity_id world_clear_entity(world *w, entity_id entity_id);

entity_id world_remove_entity(world *w, entity_id entity_id);

entity_id world_copy_entity_onto(world *w, entity_id destination, entity_id source);

entity_id world_copy_entity(world *w, entity_id destination, entity_id source);

entity_id world_add_entity_copy(world *w, entity_id source);

void *world_copy_entity_onto_and_grab(world *w, entity_id destination, entity_id source, component_id grab_component_id);
#define WORLD_COPY_ENTITY_ONTO_AND_GRAB(type, world_ref, destination, source) \
    ((type *)world_copy_entity_onto_and_grab(world_ref, destination, source, COMPONENT_ID(type)))

void *world_copy_entity_and_grab(world *w, entity_id destination, entity_id source, component_id grab_component_id);
#define WORLD_COPY_ENTITY_AND_GRAB(type, world_ref, destination, source) \
    ((type *)world_copy_entity_and_grab(world_ref, destination, source, COMPONENT_ID(type)))


void *world_set_component_relation(world *w, entity_id id, component_id component_id, void *component, size_t size, tag_id tag_id);
#define WORLD_SET_COMPONENT_RELATION(component_type, world_ref, entity_id0, component_ref, target_id) \
    world_set_component_relation(world_ref, entity_id0, COMPONENT_ID(component_type), component_ref, sizeof(component_type), target_id)

void *world_get_component_relation(const world *w, entity_id id, component_id component_id, tag_id tag_id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return world_get_component(w, id, relation_id_create(relation_id_descriptor_create_tag(component_id, tag_id)));
}
#define WORLD_GET_COMPONENT_RELATION(component_type, world_ref, entity_id0, target_id) \
    ((component_type *)world_get_component_relation(world_ref, entity_id0, COMPONENT_ID(component_type), target_id))

bool world_remove_component_relation(world *w, entity_id id, component_id component_id, void *out_removed_component, tag_id tag_id);
#define WORLD_REMOVE_COMPONENT_RELATION(component_type, world_ref, entity_id0, out_removed_component_ref, target_id) \
    world_remove_component_relation(world_ref, entity_id0, COMPONENT_ID(component_type), out_removed_component_ref, target_id)


tag_id world_add_tag_relation(world *w, entity_id id, tag_id tag, tag_id target_tag_id);
#define WORLD_ADD_TAG_RELATION(tag_type, world_ref, entity_id0, target_id) \
    world_add_tag_relation(world_ref, entity_id0, TAG_ID(tag_type), target_id)

bool world_remove_tag_relation(world *w, entity_id id, tag_id tag, tag_id target_tag_id);
#define WORLD_REMOVE_TAG_RELATION(tag_type, world_ref, entity_id0, target_id) \
    world_remove_tag_relation(world_ref, entity_id0, TAG_ID(tag_type), target_id)

associated_entity_ids_iterator world_get_associated_ids(world *w, entity_id id);


resource_handle world_set_resource(world *w, resource_id id, void *resource, size_t size);
#define WORLD_SET_RESOURCE(type, world_ref, resource_ref) \
    ((type *)world_set_resource(world_ref, RESOURCE_ID(type), resource_ref, sizeof(type)))

resource_handle world_get_resource(const world *w, resource_id id);
#define WORLD_GET_RESOURCE(type, world_ref) \
    ((type *)world_get_resource(world_ref, RESOURCE_ID(type)))

bool world_remove_resource(world *w, resource_id id);
#define WORLD_REMOVE_RESOURCE(type, world_ref) \
    world_remove_resource(world_ref, RESOURCE_ID(type))

bool world_remove_resource_out(world *w, resource_id id, resource_handle out_resource, size_t size);
#define WORLD_REMOVE_RESOURCE_OUT(type, world_ref, out_resource_ref) \
    world_remove_resource_out(world_ref, RESOURCE_ID(type), out_resource_ref, sizeof(type))

#endif