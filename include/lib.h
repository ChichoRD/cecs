#ifndef LIB_H
#define LIB_H

#include <time.h>
#include <stdint.h>
#include "core/component/entity/entity.h"
#include "core/system.h"
#include "core/world.h"
#include "containers/arena.h"
#include "containers/list.h"


typedef entity_id prefab_id;
prefab_id world_set_prefab(world *w, entity_id prefab) {
    WORLD_ADD_TAG(is_prefab, w, prefab);
    entity_flags *flags = world_get_or_set_default_entity_flags(w, prefab);
    flags->is_prefab = true;
    flags->is_inmutable = true; 
    return prefab;
}

entity_id world_unset_prefab(world *w, prefab_id prefab) {
    *world_get_or_set_default_entity_flags(w, prefab) = ENTITY_FLAGS_DEFAULT;
    WORLD_REMOVE_TAG(is_prefab, w, prefab);
    return prefab;
}

entity_id world_add_entity_from_prefab(world *w, prefab_id prefab) {
    if (world_get_entity_flags(w, prefab).is_prefab != WORLD_HAS_TAG(is_prefab, w, prefab)) {
        assert(false && "unreachable: entity flags mismatch is_prefab tag");
        exit(EXIT_FAILURE);
        return 0;
    }
    assert(
        WORLD_HAS_TAG(is_prefab, w, prefab)
        && "given entity is not a prefab"
    );

    return world_unset_prefab(
        w,
        world_add_entity_copy(w, prefab)
    );
}

entity_id world_set_components_from_prefab(world *w, entity_id destination, prefab_id prefab) {
    if (world_get_entity_flags(w, prefab).is_prefab != WORLD_HAS_TAG(is_prefab, w, prefab)) {
        assert(false && "unreachable: entity flags mismatch is_prefab tag");
        exit(EXIT_FAILURE);
        return 0;
    }
    assert(
        WORLD_HAS_TAG(is_prefab, w, prefab)
        && "given entity is not a prefab, use world_copy_entity_onto to have this behaviour copying from any kind of entity"
    );
    return world_copy_entity_onto(w, destination, prefab);
}

void *world_set_components_from_prefab_and_grab(world *w, entity_id destination, prefab_id prefab, component_id grab_component_id) {
    if (world_get_entity_flags(w, prefab).is_prefab != WORLD_HAS_TAG(is_prefab, w, prefab)) {
        assert(false && "unreachable: entity flags mismatch is_prefab tag");
        exit(EXIT_FAILURE);
        return 0;
    }
    assert(
        WORLD_HAS_TAG(is_prefab, w, prefab)
        && "given entity is not a prefab, use world_copy_entity_onto_and_grab to have this behaviour copying from any kind of entity"
    );
    return world_copy_entity_onto_and_grab(w, destination, prefab, grab_component_id);
}
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

double game_time_update_delta_time(game_time *t) {
    t->delta_time_seconds =
        ((t->frame_end.tv_sec - t->frame_start.tv_sec) + (t->frame_end.tv_nsec - t->frame_start.tv_nsec) / 1000000000.0);
    t->averaged_delta_time_seconds =
        t->averaged_delta_time_seconds
            + (t->delta_time_seconds - t->averaged_delta_time_seconds) / (t->time_since_start_seconds / t->delta_time_seconds);
    return t->delta_time_seconds;
}

double game_time_update_time_since_start(game_time *t) {
    return (t->time_since_start_seconds = 
        ((t->frame_end.tv_sec - t->game_start.tv_sec) + (t->frame_end.tv_nsec - t->game_start.tv_nsec) / 1000000000.0));
}


typedef struct is_child_of {
    entity_id parent;
} is_child_of;
COMPONENT_IMPLEMENT(is_child_of);


bool world_get_entity_with(const world *w, entity_id *out_entity_id, components_search_groups search_groups) {
    arena temporary_arena = arena_create();
    component_iterator it = component_iterator_create(component_iterator_descriptor_create(
        &w->components,
        &temporary_arena,
        search_groups
    ));

    if (!component_iterator_done(&it)) {
        *out_entity_id = component_iterator_current(&it, w->components.discard.handle);
        arena_free(&temporary_arena);
        return true;
    } else {
        arena_free(&temporary_arena);
        return false;
    }
}
#define WORLD_GET_ENTITY_WITH(world_ref, out_entity_id_ref, ...) \
    (world_get_entity_with(world_ref, out_entity_id_ref, COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__)))


typedef tag_id scene_id;

typedef bool is_scene_member_of;
TAG_IMPLEMENT(is_scene_member_of);

scene_id world_add_entity_to_scene(world *w, entity_id id, scene_id scene) {
    return WORLD_ADD_TAG_RELATION(
        is_scene_member_of,
        w,
        id,
        scene
    );
}

scene_id world_remove_entity_from_scene(world *w, entity_id id, scene_id scene) {
    return WORLD_REMOVE_TAG_RELATION(
        is_scene_member_of,
        w,
        id,
        scene
    );
}


typedef struct scene_world_system {
    dynamic_world_system world_system;
    scene_id scene;
    size_t scene_group_index;
} scene_world_system;

scene_world_system scene_world_system_create(scene_id scene, arena *a) {
    scene_world_system s = {
        .world_system = dynamic_world_system_create_from(COMPONENTS_SEARCH_GROUPS_CREATE(
            COMPONENTS_ALL_IDS(RELATION_ID(is_scene_member_of, scene))),
            a
        ),
        .scene = scene,
    };
    s.scene_group_index = LIST_COUNT(components_search_group, &s.world_system.components_search_groups) - 1;
    return s;
}

scene_world_system scene_world_system_create_from(scene_id scene, arena *a, components_search_groups search_groups) {
    scene_world_system s = scene_world_system_create(scene, a);
    dynamic_world_system_add_range(&s.world_system, a, search_groups);
    return s;
}

scene_world_system *scene_world_system_set_active_scene(scene_world_system *s, arena *a, scene_id scene) {
    s->scene = scene;
    dynamic_world_system_set(
        &s->world_system,
        a,
        COMPONENTS_ALL_IDS(RELATION_ID(is_scene_member_of, scene)),
        0
    );
    return s;
}

world_system scene_world_system_get_with(scene_world_system *s, arena *a, components_search_group search_group) {
    return dynamic_world_system_get_range(&s->world_system, dynamic_world_system_set(
        &s->world_system,
        a,
        search_group,
        s->scene_group_index + 1
    ));
}

world_system scene_world_system_get_with_range(scene_world_system *s, arena *a, components_search_groups search_groups) {
    return dynamic_world_system_get_range(&s->world_system, dynamic_world_system_set_range(
        &s->world_system,
        a,
        search_groups,
        s->scene_group_index + 1
    ));
}

#endif