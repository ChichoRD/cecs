#include "cecs_core.h"

cecs_prefab_id cecs_world_set_prefab(cecs_world* w, cecs_entity_id prefab) {
    CECS_WORLD_ADD_TAG(cecs_is_prefab, w, prefab);
    cecs_entity_flags* flags = cecs_world_get_or_set_default_entity_flags(w, prefab);
    flags->is_prefab = true;
    flags->is_inmutable = true;
    return prefab;
}
cecs_entity_id cecs_world_unset_prefab(cecs_world* w, cecs_prefab_id prefab) {
    *cecs_world_get_or_set_default_entity_flags(w, prefab) = CECS_ENTITY_FLAGS_DEFAULT;
    CECS_WORLD_REMOVE_TAG(cecs_is_prefab, w, prefab);
    return prefab;
}

cecs_entity_id cecs_world_add_entity_from_prefab(cecs_world* w, cecs_prefab_id prefab) {
    if (cecs_world_get_entity_flags(w, prefab).is_prefab != CECS_WORLD_HAS_TAG(cecs_is_prefab, w, prefab)) {
        assert(false && "unreachable: entity flags mismatch is_prefab tag");
        exit(EXIT_FAILURE);
        return 0;
    }
    assert(
        CECS_WORLD_HAS_TAG(cecs_is_prefab, w, prefab)
        && "given entity is not a prefab"
    );

    return cecs_world_unset_prefab(
        w,
        cecs_world_add_entity_copy(w, prefab)
    );
}
cecs_entity_id cecs_world_set_components_from_prefab(cecs_world* w, cecs_entity_id destination, cecs_prefab_id prefab) {
    if (cecs_world_get_entity_flags(w, prefab).is_prefab != CECS_WORLD_HAS_TAG(cecs_is_prefab, w, prefab)) {
        assert(false && "unreachable: entity flags mismatch is_prefab tag");
        exit(EXIT_FAILURE);
        return 0;
    }
    assert(
        CECS_WORLD_HAS_TAG(cecs_is_prefab, w, prefab)
        && "given entity is not a prefab, use cecs_world_copy_entity_onto to have this behaviour copying from any kind of entity"
    );
    return cecs_world_copy_entity_onto(w, destination, prefab);
}
void* cecs_world_set_components_from_prefab_and_grab(cecs_world* w, cecs_entity_id destination, cecs_prefab_id prefab, cecs_component_id grab_component_id) {
    if (cecs_world_get_entity_flags(w, prefab).is_prefab != CECS_WORLD_HAS_TAG(cecs_is_prefab, w, prefab)) {
        assert(false && "unreachable: entity flags mismatch is_prefab tag");
        exit(EXIT_FAILURE);
        return 0;
    }
    assert(
        CECS_WORLD_HAS_TAG(cecs_is_prefab, w, prefab)
        && "given entity is not a prefab, use cecs_world_copy_entity_onto_and_grab to have this behaviour copying from any kind of entity"
    );
    return cecs_world_copy_entity_onto_and_grab(w, destination, prefab, grab_component_id);
}

CECS_RESOURCE_DEFINE(cecs_game_time);

double cecs_game_time_update_delta_time(cecs_game_time* t) {
    t->delta_time_seconds =
        ((t->frame_end.tv_sec - t->frame_start.tv_sec) + (t->frame_end.tv_nsec - t->frame_start.tv_nsec) / 1000000000.0);
    t->averaged_delta_time_seconds =
        t->averaged_delta_time_seconds
        + (t->delta_time_seconds - t->averaged_delta_time_seconds) / (t->time_since_start_seconds / t->delta_time_seconds);
    return t->delta_time_seconds;
}
double cecs_game_time_update_time_since_start(cecs_game_time* t) {
    return (t->time_since_start_seconds =
        ((t->frame_end.tv_sec - t->game_start.tv_sec) + (t->frame_end.tv_nsec - t->game_start.tv_nsec) / 1000000000.0));
}

CECS_COMPONENT_DEFINE(cecs_is_child_of);
CECS_TAG_DEFINE(cecs_is_scene_member_of);

cecs_scene_id cecs_world_add_entity_to_scene(cecs_world* w, cecs_entity_id id, cecs_scene_id scene) {
    return CECS_WORLD_ADD_TAG_RELATION(
        cecs_is_scene_member_of,
        w,
        id,
        scene
    );
}
cecs_scene_id cecs_world_remove_entity_from_scene(cecs_world* w, cecs_entity_id id, cecs_scene_id scene) {
    return CECS_WORLD_REMOVE_TAG_RELATION(
        cecs_is_scene_member_of,
        w,
        id,
        scene
    );
}

cecs_scene_world_system cecs_scene_world_system_create(cecs_scene_id scene, cecs_arena* a) {
    return (cecs_scene_world_system) {
        .world_system = cecs_dynamic_world_system_create_from(
            a,
            (cecs_component_iteration_group[]){CECS_COMPONENT_GROUP_FROM_IDS(
                cecs_component_access_ignore, cecs_component_group_search_all, CECS_RELATION_ID(cecs_is_scene_member_of, scene)
            )},
            1
        ),
        .scene = scene,
        .scene_group_index = 0
    };
}

cecs_scene_world_system cecs_scene_world_system_create_from(
    const cecs_scene_id scene,
    cecs_arena *a,
    const cecs_component_iteration_group groups[const],
    const size_t group_count
) {
    cecs_scene_world_system s = cecs_scene_world_system_create(scene, a);
    cecs_dynamic_world_system_add_range(&s.world_system, a, groups, group_count);
    return s;
}

cecs_scene_world_system* cecs_scene_world_system_set_active_scene(cecs_scene_world_system* s, cecs_arena* a, const cecs_scene_id scene) {
    s->scene = scene;
    cecs_dynamic_world_system_set_or_extend_range(
        &s->world_system,
        a,
        0,
        (cecs_component_iteration_group[]){CECS_COMPONENT_GROUP_FROM_IDS(
            cecs_component_access_ignore, cecs_component_group_search_all, CECS_RELATION_ID(cecs_is_scene_member_of, scene)
        )},
        0
    );
    return s;
}