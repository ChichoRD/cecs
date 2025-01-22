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

static inline size_t cecs_world_entity_count(const cecs_world* w) {
    return cecs_world_entities_count(&w->entities);
}

void *cecs_world_get_component(const cecs_world *w, cecs_entity_id entity_id, cecs_component_id component_id);
#define CECS_WORLD_GET_COMPONENT(type, world_ref, entity_id0) \
    ((type *)cecs_world_get_component(world_ref, entity_id0, CECS_COMPONENT_ID(type)))

bool cecs_world_try_get_component(const cecs_world *w, cecs_entity_id entity_id, cecs_component_id component_id, void **out_component);
#define CECS_WORLD_TRY_GET_COMPONENT(type, world_ref, entity_id0, out_component_ref) \
    (cecs_world_try_get_component(world_ref, entity_id0, CECS_COMPONENT_ID(type), ((void **)out_component_ref)))

size_t cecs_world_get_component_array(const cecs_world *w, cecs_entity_id_range range, cecs_component_id component_id, void **out_components);
#define CECS_WORLD_GET_COMPONENT_ARRAY(type, world_ref, entity_id_range, out_components_ref) \
    (cecs_world_get_component_array(world_ref, entity_id_range, CECS_COMPONENT_ID(type), ((void **)out_components_ref)))

cecs_entity_flags cecs_world_get_entity_flags(const cecs_world *w, cecs_entity_id entity_id);

void *cecs_world_set_component(cecs_world *w, cecs_entity_id id, cecs_component_id component_id, void *component, size_t size);
#define CECS_WORLD_SET_COMPONENT(type, world_ref, entity_id0, component_ref) \
    ((type *)cecs_world_set_component(world_ref, entity_id0, CECS_COMPONENT_ID(type), component_ref, sizeof(type)))

void *cecs_world_set_component_array(cecs_world *w, cecs_entity_id_range range, cecs_component_id component_id, void *components, size_t size);
#define CECS_WORLD_SET_COMPONENT_ARRAY(type, world_ref, entity_id_range, components_ref) \
    ((type *)cecs_world_set_component_array(world_ref, entity_id_range, CECS_COMPONENT_ID(type), components_ref, sizeof(type)))

void *cecs_world_set_component_copy_array(cecs_world *w, cecs_entity_id_range range, cecs_component_id component_id, void *component_single_src, size_t size);
#define CECS_WORLD_SET_COMPONENT_COPY_ARRAY(type, world_ref, entity_id_range, component_single_src_ref) \
    ((type *)cecs_world_set_component_copy_array(world_ref, entity_id_range, CECS_COMPONENT_ID(type), component_single_src_ref, sizeof(type)))

bool cecs_world_remove_component(cecs_world *w, cecs_entity_id id, cecs_component_id component_id, void *out_removed_component);
#define CECS_WORLD_REMOVE_COMPONENT(type, world_ref, entity_id0, out_removed_component_ref) \
    (cecs_world_remove_component(world_ref, entity_id0, CECS_COMPONENT_ID(type), out_removed_component_ref))

size_t cecs_world_remove_component_array(cecs_world *w, cecs_entity_id_range range, cecs_component_id component_id, void *out_removed_components);
#define CECS_WORLD_REMOVE_COMPONENT_ARRAY(type, world_ref, entity_id_range, out_removed_components_ref) \
    (cecs_world_remove_component_array(world_ref, entity_id_range, CECS_COMPONENT_ID(type), out_removed_components_ref))

static inline cecs_entity_flags *cecs_world_set_entity_flags(cecs_world *w, cecs_entity_id id, cecs_entity_flags flags) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return CECS_WORLD_SET_COMPONENT(cecs_entity_flags, w, id, &flags);
}

void *cecs_world_set_component_storage_attachments(cecs_world *w, cecs_component_id component_id, void *attachments, size_t size);
#define CECS_WORLD_SET_COMPONENT_STORAGE_ATTACHMENTS(type, attachment_type, world_ref, attachments_ref) \
    ((attachment_type *)cecs_world_set_component_storage_attachments(world_ref, CECS_COMPONENT_ID(type), attachments_ref, sizeof(type)))
bool cecs_world_has_component_storage_attachments(const cecs_world *w, cecs_component_id component_id);
#define CECS_WORLD_HAS_COMPONENT_STORAGE_ATTACHMENTS(type, world_ref) \
    cecs_world_has_component_storage_attachments(world_ref, CECS_COMPONENT_ID(type))
void *cecs_world_get_component_storage_attachments(const cecs_world *w, cecs_component_id component_id);
#define CECS_WORLD_GET_COMPONENT_STORAGE_ATTACHMENTS(type, attachment_type, world_ref) \
    ((attachment_type *)cecs_world_get_component_storage_attachments(world_ref, CECS_COMPONENT_ID(type)))
void *cecs_world_get_or_set_component_storage_attachments(cecs_world *w, cecs_component_id component_id, void *default_attachments, size_t size);
#define CECS_WORLD_GET_OR_SET_COMPONENT_STORAGE_ATTACHMENTS(type, attachment_type, world_ref, default_attachments_ref) \
    ((attachment_type *)cecs_world_get_or_set_component_storage_attachments(world_ref, CECS_COMPONENT_ID(type), default_attachments_ref, sizeof(type)))
bool cecs_world_remove_component_storage_attachments(cecs_world *w, cecs_component_id component_id, void *out_removed_attachments);
#define CECS_WORLD_REMOVE_COMPONENT_STORAGE_ATTACHMENTS(type, world_ref, out_removed_attachments_ref) \
    cecs_world_remove_component_storage_attachments(world_ref, CECS_COMPONENT_ID(type), out_removed_attachments_ref)


cecs_entity_flags *cecs_world_get_or_set_default_entity_flags(cecs_world *w, cecs_entity_id id);

bool cecs_world_has_tag(const cecs_world *w, cecs_entity_id id, cecs_tag_id tag_id);
#define CECS_WORLD_HAS_TAG(type, world_ref, entity_id0) \
    cecs_world_has_tag(world_ref, entity_id0, CECS_TAG_ID(type))

cecs_tag_id cecs_world_add_tag(cecs_world *w, cecs_entity_id id, cecs_tag_id tag_id);
#define CECS_WORLD_ADD_TAG(type, world_ref, entity_id0) \
    cecs_world_add_tag(world_ref, entity_id0, CECS_TAG_ID(type))

cecs_tag_id cecs_world_add_tag_array(cecs_world *w, cecs_entity_id_range range, cecs_tag_id tag_id);
#define CECS_WORLD_ADD_TAG_ARRAY(type, world_ref, entity_id_range) \
    cecs_world_add_tag_array(world_ref, entity_id_range, CECS_TAG_ID(type))

cecs_tag_id cecs_world_remove_tag(cecs_world *w, cecs_entity_id id, cecs_tag_id tag_id);
#define CECS_WORLD_REMOVE_TAG(type, world_ref, entity_id0) \
    cecs_world_remove_tag(world_ref, entity_id0, CECS_TAG_ID(type))

size_t cecs_world_remove_tag_array(cecs_world *w, cecs_entity_id_range range, cecs_tag_id tag_id);
#define CECS_WORLD_REMOVE_TAG_ARRAY(type, world_ref, entity_id_range) \
    cecs_world_remove_tag_array(world_ref, entity_id_range, CECS_TAG_ID(type))

void *cecs_world_use_component_discard(cecs_world *w, size_t size);
void *cecs_world_use_resource_discard(cecs_world *w, size_t size);

cecs_entity_id cecs_world_add_entity(cecs_world *w);
cecs_entity_id cecs_world_clear_entity(cecs_world *w, cecs_entity_id entity_id);

// TODO: ensure relation are accounted for
cecs_entity_id cecs_world_remove_entity(cecs_world *w, cecs_entity_id entity_id);

cecs_entity_id cecs_world_copy_entity_onto(cecs_world *w, cecs_entity_id destination, cecs_entity_id source);
cecs_entity_id cecs_world_copy_entity(cecs_world *w, cecs_entity_id destination, cecs_entity_id source);

cecs_entity_id cecs_world_add_entity_copy(cecs_world *w, cecs_entity_id source);

cecs_entity_id_range cecs_world_add_entity_range(cecs_world *w, size_t count);
size_t cecs_world_clear_entity_range(cecs_world *w, cecs_entity_id_range range, cecs_entity_id representative);

cecs_entity_id_range cecs_world_remove_entity_range(cecs_world *w, cecs_entity_id_range range);

// TODO: range operations work but test more thoroughly just in case
size_t cecs_world_copy_entity_range_onto(cecs_world *w, cecs_entity_id_range destination, cecs_entity_id_range source, cecs_entity_id representative);
size_t cecs_world_copy_entity_range(
    cecs_world *w,
    cecs_entity_id_range destination,
    cecs_entity_id_range source,
    cecs_entity_id clear_representative,
    cecs_entity_id copy_representative
);

void *cecs_world_copy_entity_onto_and_grab(cecs_world *w, cecs_entity_id destination, cecs_entity_id source, cecs_component_id grab_component_id);
#define CECS_WORLD_COPY_ENTITY_ONTO_AND_GRAB(type, world_ref, destination, source) \
    ((type *)cecs_world_copy_entity_onto_and_grab(world_ref, destination, source, CECS_COMPONENT_ID(type)))

void *cecs_world_copy_entity_and_grab(cecs_world *w, cecs_entity_id destination, cecs_entity_id source, cecs_component_id grab_component_id);
#define CECS_WORLD_COPY_ENTITY_AND_GRAB(type, world_ref, destination, source) \
    ((type *)cecs_world_copy_entity_and_grab(world_ref, destination, source, CECS_COMPONENT_ID(type)))

void *cecs_world_set_component_relation(cecs_world *w, cecs_entity_id id, cecs_component_id component_id, void *component, size_t size, cecs_tag_id tag_id);
#define CECS_WORLD_SET_COMPONENT_RELATION(component_type, world_ref, entity_id0, component_ref, target_id) \
    ((component_type *)cecs_world_set_component_relation(world_ref, entity_id0, CECS_COMPONENT_ID(component_type), component_ref, sizeof(component_type), target_id))

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