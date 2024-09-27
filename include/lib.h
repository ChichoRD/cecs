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

#endif