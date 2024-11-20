#ifndef LIB_H
#define LIB_H

#include <time.h>
#include <stdint.h>
#include "core/component/entity/entity.h"
#include "core/system.h"
#include "core/world.h"
#include "containers/cecs_arena.h"
#include "containers/cecs_dynamic_array.h"


typedef entity_id prefab_id;
prefab_id world_set_prefab(world *w, entity_id prefab);

entity_id world_unset_prefab(world *w, prefab_id prefab);

entity_id world_add_entity_from_prefab(world *w, prefab_id prefab);

entity_id world_set_components_from_prefab(world *w, entity_id destination, prefab_id prefab);

void *world_set_components_from_prefab_and_grab(world *w, entity_id destination, prefab_id prefab, component_id grab_component_id);
#define WORLD_SET_COMPONENTS_FROM_PREFAB_AND_GRAB(type, world_ref, destination, prefab) \
    ((type *)world_set_components_from_prefab_and_grab(world_ref, destination, prefab, COMPONENT_ID(type)))



typedef struct game_time {
    struct timespec game_start;
    struct timespec frame_start;
    struct timespec frame_end;
    double delta_time_seconds;
    double averaged_delta_time_seconds;
    double time_since_start_seconds;
} game_time;
RESOURCE_IMPLEMENT(game_time);

double game_time_update_delta_time(game_time *t);

double game_time_update_time_since_start(game_time *t);


typedef struct is_child_of {
    entity_id parent;
} is_child_of;
COMPONENT_IMPLEMENT(is_child_of);


bool world_get_entity_with(const world *w, entity_id *out_entity_id, components_search_groups search_groups);
#define WORLD_GET_ENTITY_WITH(world_ref, out_entity_id_ref, ...) \
    (world_get_entity_with(world_ref, out_entity_id_ref, COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__)))


typedef tag_id scene_id;

typedef bool is_scene_member_of;
TAG_IMPLEMENT(is_scene_member_of);

scene_id world_add_entity_to_scene(world *w, entity_id id, scene_id scene);

scene_id world_remove_entity_from_scene(world *w, entity_id id, scene_id scene);

typedef struct scene_world_system {
    dynamic_world_system world_system;
    scene_id scene;
    size_t scene_group_index;
} scene_world_system;

scene_world_system scene_world_system_create(scene_id scene, cecs_arena *a);

scene_world_system scene_world_system_create_from(scene_id scene, cecs_arena *a, components_search_groups search_groups);

scene_world_system *scene_world_system_set_active_scene(scene_world_system *s, cecs_arena *a, scene_id scene);

world_system scene_world_system_get_with(scene_world_system *s, cecs_arena *a, components_search_group search_group);

world_system scene_world_system_get_with_range(scene_world_system *s, cecs_arena *a, components_search_groups search_groups);

#endif