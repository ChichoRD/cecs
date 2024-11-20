#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>
#include <stdlib.h>
#include "../containers/tagged_union.h"
#include "component/component_iterator.h"
#include "world.h"

typedef struct world_system {
    components_search_groups const search_groups;
} world_system;

inline world_system world_system_create(components_search_groups search_groups) {
    return (world_system){ 
        .search_groups = search_groups
    };
}
#define WORLD_SYSTEM_CREATE_GROUPED(...) world_system_create(COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__))
#define WORLD_SYSTEM_CREATE_GROUPED_FROM_IDS(...) world_system_create(COMPONENTS_SEARCH_GROUPS_CREATE(__VA_ARGS__))

#define WORLD_SYSTEM_CREATE(...) WORLD_SYSTEM_CREATE_GROUPED(COMPONENTS_ALL(__VA_ARGS__))
#define WORLD_SYSTEM_CREATE_FROM_IDS(...) WORLD_SYSTEM_CREATE_GROUPED(COMPONENTS_ALL_IDS(__VA_ARGS__))

typedef size_t entity_count;
typedef TAGGED_UNION_STRUCT(
    system_predicate_data,
    none,
    none,
    double,
    delta_time,
    void *,
    user_data
) system_predicate_data;

inline system_predicate_data system_predicate_data_create_none() {
    return (system_predicate_data)TAGGED_UNION_CREATE(none, system_predicate_data, NONE);
}

inline system_predicate_data system_predicate_data_create_delta_time(double delta_time_seconds) {
    return (system_predicate_data)TAGGED_UNION_CREATE(delta_time, system_predicate_data, delta_time_seconds);
}

inline system_predicate_data system_predicate_data_create_user_data(void *data) {
    return (system_predicate_data)TAGGED_UNION_CREATE(user_data, system_predicate_data, data);
}

inline double system_predicate_data_delta_time(system_predicate_data data) {
    return TAGGED_UNION_GET_UNCHECKED(delta_time, data);
}

inline void *system_predicate_data_user_data(system_predicate_data data) {
    return TAGGED_UNION_GET_UNCHECKED(user_data, data);
}

typedef void system_predicate(const raw_iteration_handle_reference handle, world *world, system_predicate_data data);
entity_count world_system_iter(
    const world_system s,
    world *w,
    cecs_arena *iteration_arena,
    system_predicate_data data,
    system_predicate *predicate
);
#define WORLD_SYSTEM_ITER(world_system0, world_ref, iteration_arena_ref, predicate_data, predicate) \
    world_system_iter(world_system0, world_ref, iteration_arena_ref, predicate_data, ((system_predicate *)predicate))

entity_count world_system_iter_range(
    const world_system s,
    world *w,
    cecs_arena *iteration_arena,
    entity_id_range range,
    system_predicate_data data,
    system_predicate *predicate
);
#define WORLD_SYSTEM_ITER_RANGE(world_system0, world_ref, iteration_arena_ref, entity_id_range0, predicate_data, predicate) \
    world_system_iter_range( \
        world_system0, world_ref, iteration_arena_ref, entity_id_range0, predicate_data, ((system_predicate *)predicate) \
    )

typedef struct system_predicates {
    system_predicate **predicates;
    size_t predicate_count;
} system_predicates;
system_predicates system_predicates_create(system_predicate **predicates, size_t predicate_count);
#define SYSTEM_PREDICATES_CREATE(...) \
    system_predicates_create( \
        (system_predicate *[]) { __VA_ARGS__ }, \
        sizeof((system_predicate *[]) { __VA_ARGS__ }) / sizeof(((system_predicate *[]) { __VA_ARGS__ })[0]) \
    )

typedef union {
    struct {
        system_predicate **predicates;
        size_t predicate_count;
    };
    system_predicates system_predicates;
} system_predicates_deconstruct;

entity_count world_system_iter_all(
    const world_system s,
    world *w,
    cecs_arena *iteration_arena,
    system_predicate_data data,
    system_predicates predicates
);
#define WORLD_SYSTEM_ITER_ALL(world_system0, world_ref, iteration_arena_ref, predicate_data, ...) \
    world_system_iter_all(world_system0, world_ref, iteration_arena_ref, predicate_data, SYSTEM_PREDICATES_CREATE(__VA_ARGS__))

entity_count world_system_iter_range_all(
    const world_system s,
    world *w,
    cecs_arena *iteration_arena,
    entity_id_range range,
    system_predicate_data data,
    system_predicates predicates
);
#define WORLD_SYSTEM_ITER_RANGE_ALL(world_system0, world_ref, iteration_arena_ref, entity_id_range0, predicate_data, ...) \
    world_system_iter_range_all( \
        world_system0, world_ref, iteration_arena_ref, entity_id_range0, predicate_data, SYSTEM_PREDICATES_CREATE(__VA_ARGS__) \
    )


typedef struct dynamic_world_system {
    list component_ids;
    list components_search_groups;
} dynamic_world_system;

dynamic_world_system dynamic_world_system_create();

dynamic_world_system dynamic_world_system_create_from(components_search_groups s, cecs_arena *a);

typedef exclusive_range components_search_group_range; 
components_search_group_range dynamic_world_system_add(dynamic_world_system *d, cecs_arena *a, components_search_group s);

components_search_group_range dynamic_world_system_add_range(dynamic_world_system *d, cecs_arena *a, components_search_groups s);

components_search_group_range dynamic_world_system_set(dynamic_world_system *d, cecs_arena *a, components_search_group s, size_t index);

components_search_group_range dynamic_world_system_set_range(dynamic_world_system *d, cecs_arena *a, components_search_groups s, size_t index);

world_system dynamic_world_system_get(const dynamic_world_system *d);

world_system dynamic_world_system_get_range(const dynamic_world_system *d, components_search_group_range r);

dynamic_world_system dynamic_world_system_clone(const dynamic_world_system *d, cecs_arena *a);

#endif