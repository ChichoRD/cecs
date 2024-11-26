#ifndef CECS_WORLD_H
#define CECS_WORLD_H

#include <assert.h>
#include <stdbool.h>
#include "../containers/cecs_arena.h"
#include "component/entity/cecs_entity.h"
#include "component/entity/cecs_tag.h"
#include "component/entity/cecs_game_component.h"
#include "component/cecs_component.h"
#include "resource/cecs_resource.h"
#include "cecs_relation.h"

#define CECS_WORLD_UNIQUE_RELATION_COMPONENTS true
#define CECS_WORLD_FLAG_ALL_ENTITIES true

typedef struct cecs_world {
    cecs_world_entities entities;
    cecs_world_components components;
    cecs_world_relations relations;
    cecs_world_resources resources;
} cecs_world;

cecs_world cecs_world_create(size_t entity_capacity, size_t component_type_capacity, size_t resource_capacity);

void cecs_world_free(cecs_world *w);

inline size_t cecs_world_entity_count(const cecs_world* w) {
    return cecs_world_entities_count(&w->entities);
}

void *cecs_world_get_component(const cecs_world *w, cecs_entity_id entity_id, cecs_component_id component_id);
#define CECS_WORLD_GET_COMPONENT(type, world_ref, entity_id0) \
    ((type *)cecs_world_get_component(world_ref, entity_id0, CECS_COMPONENT_ID(type)))

bool cecs_world_try_get_component(const cecs_world *w, cecs_entity_id entity_id, cecs_component_id component_id, void **out_component);
#define CECS_WORLD_TRY_GET_COMPONENT(type, world_ref, entity_id0, out_component_ref) \
    (cecs_world_try_get_component(world_ref, entity_id0, CECS_COMPONENT_ID(type), ((void **)out_component_ref)))

cecs_entity_flags cecs_world_get_entity_flags(const cecs_world *w, cecs_entity_id entity_id);

void *cecs_world_set_component(cecs_world *w, cecs_entity_id id, cecs_component_id component_id, void *component, size_t size);
#define CECS_WORLD_SET_COMPONENT(type, world_ref, entity_id0, component_ref) \
    ((type *)cecs_world_set_component(world_ref, entity_id0, CECS_COMPONENT_ID(type), component_ref, sizeof(type)))

bool cecs_world_remove_component(cecs_world *w, cecs_entity_id id, cecs_component_id component_id, void *out_removed_component);
#define CECS_WORLD_REMOVE_COMPONENT(type, world_ref, entity_id0, out_removed_component_ref) \
    (cecs_world_remove_component(world_ref, entity_id0, CECS_COMPONENT_ID(type), out_removed_component_ref))

static inline cecs_entity_flags *cecs_world_set_entity_flags(cecs_world *w, cecs_entity_id id, cecs_entity_flags flags) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return CECS_WORLD_SET_COMPONENT(cecs_entity_flags, w, id, &flags);
}

cecs_entity_flags *cecs_world_get_or_set_default_entity_flags(cecs_world *w, cecs_entity_id id);

bool cecs_world_has_tag(const cecs_world *w, cecs_entity_id id, cecs_tag_id tag_id);
#define CECS_WORLD_HAS_TAG(type, world_ref, entity_id0) \
    cecs_world_has_tag(world_ref, entity_id0, CECS_TAG_ID(type))

cecs_tag_id cecs_world_add_tag(cecs_world *w, cecs_entity_id id, cecs_tag_id tag_id);
#define CECS_WORLD_ADD_TAG(type, world_ref, entity_id0) \
    cecs_world_add_tag(world_ref, entity_id0, CECS_TAG_ID(type))

cecs_tag_id cecs_world_remove_tag(cecs_world *w, cecs_entity_id id, cecs_tag_id tag_id);
#define CECS_WORLD_REMOVE_TAG(type, world_ref, entity_id0) \
    cecs_world_remove_tag(world_ref, entity_id0, CECS_TAG_ID(type))

cecs_entity_id cecs_world_add_entity(cecs_world *w);

cecs_entity_id cecs_world_clear_entity(cecs_world *w, cecs_entity_id entity_id);

cecs_entity_id cecs_world_remove_entity(cecs_world *w, cecs_entity_id entity_id);

cecs_entity_id cecs_world_copy_entity_onto(cecs_world *w, cecs_entity_id destination, cecs_entity_id source);

cecs_entity_id cecs_world_copy_entity(cecs_world *w, cecs_entity_id destination, cecs_entity_id source);

cecs_entity_id cecs_world_add_entity_copy(cecs_world *w, cecs_entity_id source);

void *cecs_world_copy_entity_onto_and_grab(cecs_world *w, cecs_entity_id destination, cecs_entity_id source, cecs_component_id grab_component_id);
#define CECS_WORLD_COPY_ENTITY_ONTO_AND_GRAB(type, world_ref, destination, source) \
    ((type *)cecs_world_copy_entity_onto_and_grab(world_ref, destination, source, CECS_COMPONENT_ID(type)))

void *cecs_world_copy_entity_and_grab(cecs_world *w, cecs_entity_id destination, cecs_entity_id source, cecs_component_id grab_component_id);
#define CECS_WORLD_COPY_ENTITY_AND_GRAB(type, world_ref, destination, source) \
    ((type *)cecs_world_copy_entity_and_grab(world_ref, destination, source, CECS_COMPONENT_ID(type)))

void *cecs_world_set_component_relation(cecs_world *w, cecs_entity_id id, cecs_component_id component_id, void *component, size_t size, cecs_tag_id tag_id);
#define CECS_WORLD_SET_COMPONENT_RELATION(component_type, world_ref, entity_id0, component_ref, target_id) \
    cecs_world_set_component_relation(world_ref, entity_id0, CECS_COMPONENT_ID(component_type), component_ref, sizeof(component_type), target_id)

void *cecs_world_get_component_relation(const cecs_world *w, cecs_entity_id id, cecs_component_id component_id, cecs_tag_id tag_id);
#define CECS_WORLD_GET_COMPONENT_RELATION(component_type, world_ref, entity_id0, target_id) \
    ((component_type *)cecs_world_get_component_relation(world_ref, entity_id0, CECS_COMPONENT_ID(component_type), target_id))

bool cecs_world_remove_component_relation(cecs_world *w, cecs_entity_id id, cecs_component_id component_id, void *out_removed_component, cecs_tag_id tag_id);
#define CECS_WORLD_REMOVE_COMPONENT_RELATION(component_type, world_ref, entity_id0, out_removed_component_ref, target_id) \
    cecs_world_remove_component_relation(world_ref, entity_id0, CECS_COMPONENT_ID(component_type), out_removed_component_ref, target_id)

cecs_tag_id cecs_world_add_tag_relation(cecs_world *w, cecs_entity_id id, cecs_tag_id tag, cecs_tag_id target_tag_id);
#define CECS_WORLD_ADD_TAG_RELATION(tag_type, world_ref, entity_id0, target_id) \
    cecs_world_add_tag_relation(world_ref, entity_id0, CECS_TAG_ID(tag_type), target_id)

bool cecs_world_remove_tag_relation(cecs_world *w, cecs_entity_id id, cecs_tag_id tag, cecs_tag_id target_tag_id);
#define CECS_WORLD_REMOVE_TAG_RELATION(tag_type, world_ref, entity_id0, target_id) \
    cecs_world_remove_tag_relation(world_ref, entity_id0, CECS_TAG_ID(tag_type), target_id)

cecs_associated_entity_ids_iterator cecs_world_get_associated_ids(cecs_world *w, cecs_entity_id id);

cecs_resource_handle cecs_world_set_resource(cecs_world *w, cecs_resource_id id, void *resource, size_t size);
#define CECS_WORLD_SET_RESOURCE(type, world_ref, resource_ref) \
    ((type *)cecs_world_set_resource(world_ref, CECS_RESOURCE_ID(type), resource_ref, sizeof(type)))

cecs_resource_handle cecs_world_get_resource(const cecs_world *w, cecs_resource_id id);
#define CECS_WORLD_GET_RESOURCE(type, world_ref) \
    ((type *)cecs_world_get_resource(world_ref, CECS_RESOURCE_ID(type)))

bool cecs_world_remove_resource(cecs_world *w, cecs_resource_id id);
#define CECS_WORLD_REMOVE_RESOURCE(type, world_ref) \
    cecs_world_remove_resource(world_ref, CECS_RESOURCE_ID(type))

bool cecs_world_remove_resource_out(cecs_world *w, cecs_resource_id id, cecs_resource_handle out_resource, size_t size);
#define CECS_WORLD_REMOVE_RESOURCE_OUT(type, world_ref, out_resource_ref) \
    cecs_world_remove_resource_out(world_ref, CECS_RESOURCE_ID(type), out_resource_ref, sizeof(type))

#endif