#include "cecs.h"

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

bool cecs_world_get_entity_with(const cecs_world* w, cecs_entity_id* out_entity_id, cecs_components_search_groups search_groups) {
    cecs_arena temporary_arena = cecs_arena_create();
    cecs_component_iterator it = cecs_component_iterator_create(cecs_component_iterator_descriptor_create(
        &w->components,
        &temporary_arena,
        search_groups
    ));

    if (!cecs_component_iterator_done(&it)) {
        *out_entity_id = cecs_component_iterator_current(&it, w->components.discard.handle);
        cecs_arena_free(&temporary_arena);
        return true;
    }
    else {
        cecs_arena_free(&temporary_arena);
        return false;
    }
}

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
    cecs_scene_world_system s = {
        .world_system = cecs_dynamic_world_system_create_from(CECS_COMPONENTS_SEARCH_GROUPS_CREATE(
            CECS_COMPONENTS_ALL_IDS(CECS_RELATION_ID(cecs_is_scene_member_of, scene))),
            a
        ),
        .scene = scene,
    };
    s.scene_group_index = CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &s.world_system.components_search_groups) - 1;
    return s;
}

cecs_scene_world_system cecs_scene_world_system_create_from(cecs_scene_id scene, cecs_arena* a, cecs_components_search_groups search_groups) {
    cecs_scene_world_system s = cecs_scene_world_system_create(scene, a);
    cecs_dynamic_world_system_add_range(&s.world_system, a, search_groups);
    return s;
}

cecs_scene_world_system* cecs_scene_world_system_set_active_scene(cecs_scene_world_system* s, cecs_arena* a, cecs_scene_id scene) {
    s->scene = scene;
    cecs_dynamic_world_system_set(
        &s->world_system,
        a,
        CECS_COMPONENTS_ALL_IDS(CECS_RELATION_ID(cecs_is_scene_member_of, scene)),
        0
    );
    return s;
}

cecs_world_system cecs_scene_world_system_get_with(cecs_scene_world_system* s, cecs_arena* a, cecs_components_search_group search_group) {
    return cecs_dynamic_world_system_get_range(&s->world_system, cecs_dynamic_world_system_set(
        &s->world_system,
        a,
        search_group,
        s->scene_group_index + 1
    ));
}

cecs_world_system cecs_scene_world_system_get_with_range(cecs_scene_world_system* s, cecs_arena* a, cecs_components_search_groups search_groups) {
    return cecs_dynamic_world_system_get_range(&s->world_system, cecs_dynamic_world_system_set_range(
        &s->world_system,
        a,
        search_groups,
        s->scene_group_index + 1
    ));
}
