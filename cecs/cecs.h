#ifndef CECS_LIB_H
#define CECS_LIB_H

#include <time.h>
#include <stdint.h>
#include "core/component/entity/cecs_entity.h"
#include "core/cecs_system.h"
#include "core/cecs_world.h"
#include "containers/cecs_arena.h"

typedef cecs_entity_id cecs_prefab_id;
cecs_prefab_id cecs_world_set_prefab(cecs_world *w, cecs_entity_id prefab);

cecs_entity_id cecs_world_unset_prefab(cecs_world *w, cecs_prefab_id prefab);

cecs_entity_id cecs_world_add_entity_from_prefab(cecs_world *w, cecs_prefab_id prefab);

cecs_entity_id cecs_world_set_components_from_prefab(cecs_world *w, cecs_entity_id destination, cecs_prefab_id prefab);

void *cecs_world_set_components_from_prefab_and_grab(cecs_world *w, cecs_entity_id destination, cecs_prefab_id prefab, cecs_component_id grab_component_id);
#define CECS_WORLD_SET_COMPONENTS_FROM_PREFAB_AND_GRAB(type, world_ref, destination, prefab) \
    ((type *)cecs_world_set_components_from_prefab_and_grab(world_ref, destination, prefab, CECS_COMPONENT_ID(type)))

typedef struct cecs_game_time {
    struct timespec game_start;
    struct timespec frame_start;
    struct timespec frame_end;
    double delta_time_seconds;
    double averaged_delta_time_seconds;
    double time_since_start_seconds;
} cecs_game_time;
CECS_RESOURCE_IMPLEMENT(cecs_game_time);

double cecs_game_time_update_delta_time(cecs_game_time *t);

double cecs_game_time_update_time_since_start(cecs_game_time *t);

typedef struct cecs_is_child_of {
    cecs_entity_id parent;
} cecs_is_child_of;
CECS_COMPONENT_IMPLEMENT(cecs_is_child_of);

bool cecs_world_get_entity_with(const cecs_world *w, cecs_entity_id *out_entity_id, cecs_components_search_groups search_groups);
#define CECS_WORLD_GET_ENTITY_WITH(world_ref, out_entity_id_ref, ...) \
    (cecs_world_get_entity_with(world_ref, out_entity_id_ref, CECS_COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__)))

typedef cecs_tag_id cecs_scene_id;

typedef bool cecs_is_scene_member_of;
CECS_TAG_IMPLEMENT(cecs_is_scene_member_of);

cecs_scene_id cecs_world_add_entity_to_scene(cecs_world *w, cecs_entity_id id, cecs_scene_id scene);

cecs_scene_id cecs_world_remove_entity_from_scene(cecs_world *w, cecs_entity_id id, cecs_scene_id scene);

typedef struct cecs_scene_world_system {
    cecs_dynamic_world_system world_system;
    cecs_scene_id scene;
    size_t scene_group_index;
} cecs_scene_world_system;

cecs_scene_world_system cecs_scene_world_system_create(cecs_scene_id scene, cecs_arena *a);

cecs_scene_world_system cecs_scene_world_system_create_from(cecs_scene_id scene, cecs_arena *a, cecs_components_search_groups search_groups);

cecs_scene_world_system *cecs_scene_world_system_set_active_scene(cecs_scene_world_system *s, cecs_arena *a, cecs_scene_id scene);

cecs_world_system cecs_scene_world_system_get_with(cecs_scene_world_system *s, cecs_arena *a, cecs_components_search_group search_group);

cecs_world_system cecs_scene_world_system_get_with_range(cecs_scene_world_system *s, cecs_arena *a, cecs_components_search_groups search_groups);

#endif