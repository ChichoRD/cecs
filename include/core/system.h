#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>
#include <stdlib.h>
#include "containers/tagged_union.h"
#include "component/component_iterator.h"
#include "world.h"

typedef struct world_system {
    components_search_groups search_groups;
    //list subsystems;
} world_system;

world_system world_system_create(components_search_groups search_groups) {
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
    arena *iteration_arena,
    system_predicate_data data,
    system_predicate *const predicate
) {
    entity_count count = 0;
    component_iterator_descriptor descriptor = component_iterator_descriptor_create(&w->components, iteration_arena, s.search_groups);
    raw_iteration_handle_reference handle = component_iterator_descriptor_allocate_handle(descriptor);
    for (
        component_iterator it = component_iterator_create(descriptor);
        !component_iterator_done(&it);
        component_iterator_next(&it)
    ) {
        ++count;
        component_iterator_current(&it, handle);
        predicate(handle, w, data);
    }
    free(handle);
    return count;
}
#define WORLD_SYSTEM_ITER(world_system0, world_ref, iteration_arena_ref, predicate_data, predicate) \
    world_system_iter(world_system0, world_ref, iteration_arena_ref, predicate_data, ((system_predicate *const)predicate))

typedef struct system_predicates {
    system_predicate *const *const predicates;
    const size_t predicate_count;
} system_predicates;
system_predicates system_predicates_create(const system_predicate *const *predicates, size_t predicate_count) {
    return (system_predicates){
        .predicates = predicates,
        .predicate_count = predicate_count
    };
}
#define SYSTEM_PREDICATES_CREATE(...) \
    system_predicates_create( \
        (system_predicate *[]) { __VA_ARGS__ }, \
        sizeof((system_predicate *[]) { __VA_ARGS__ }) / sizeof(((system_predicate *[]) { __VA_ARGS__ })[0]) \
    )

typedef union {
    struct {
        const system_predicate *const *predicates;
        const size_t predicate_count;
    };
    system_predicates system_predicates;
} system_predicates_deconstruct;

entity_count world_system_iter_all(
    const world_system s,
    world *w,
    arena *iteration_arena,
    system_predicate_data data,
    system_predicates predicates
) {
    entity_count count = 0;
    component_iterator_descriptor descriptor = component_iterator_descriptor_create(&w->components, iteration_arena, s.search_groups);
    raw_iteration_handle_reference handle = component_iterator_descriptor_allocate_handle(descriptor);
    for (
        component_iterator it = component_iterator_create(descriptor);
        !component_iterator_done(&it);
        component_iterator_next(&it)
    ) {
        ++count;
        component_iterator_current(&it, handle);
        for (size_t i = 0; i < predicates.predicate_count; i++) {
            predicates.predicates[i](handle, w, data);
        }
    }
    free(handle);
    return count;
}
#define WORLD_SYSTEM_ITER_ALL(world_system0, world_ref, iteration_arena_ref, predicate_data, ...) \
    world_system_iter_all(world_system0, world_ref, iteration_arena_ref, predicate_data, SYSTEM_PREDICATES_CREATE(__VA_ARGS__))


typedef struct dynamic_world_system {
    list component_ids;
    list components_search_groups;
} dynamic_world_system;

dynamic_world_system dynamic_world_system_create() {
    return (dynamic_world_system){
        .component_ids = list_create(),
        .components_search_groups = list_create(),
    };
}

dynamic_world_system dynamic_world_system_create_from(components_search_groups s, arena *a) {
    dynamic_world_system d = dynamic_world_system_create();
    for (size_t i = 0; i < s.group_count; ++i) {
        components_search_group group = s.groups[i];
        components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, group);
        TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, group).component_ids =
            LIST_ADD_RANGE(component_id, &d.component_ids, a, info.component_ids, info.component_count);
        LIST_ADD(components_search_group, &d.components_search_groups, a, &group);
    }
    return d;
}

typedef exclusive_range components_search_group_range; 
components_search_group_range dynamic_world_system_add(dynamic_world_system *d, arena *a, components_search_group s) {
    components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, s);
    TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, s).component_ids =
        LIST_ADD_RANGE(component_id, &d->component_ids, a, info.component_ids, info.component_count);

    LIST_ADD(components_search_group, &d->components_search_groups, a, &s);
    return (components_search_group_range){ 0, LIST_COUNT(components_search_group, &d->components_search_groups) };
}

components_search_group_range dynamic_world_system_add_range(dynamic_world_system *d, arena *a, components_search_groups s) {
    for (size_t i = 0; i < s.group_count; ++i) {
        components_search_group group = s.groups[i];
        components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, group);
        TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, group).component_ids =
            LIST_ADD_RANGE(component_id, &d->component_ids, a, info.component_ids, info.component_count);
        LIST_ADD(components_search_group, &d->components_search_groups, a, &group);
    }
    return (components_search_group_range){ 0, LIST_COUNT(components_search_group, &d->components_search_groups) };
}

components_search_group_range dynamic_world_system_set(dynamic_world_system *d, arena *a, components_search_group s, size_t index) {
    assert(index <= LIST_COUNT(components_search_group, &d->components_search_groups) && "index out of bounds");
    if (index == LIST_COUNT(components_search_group, &d->components_search_groups)) {
        dynamic_world_system_add(d, a, s);
        return (components_search_group_range){ 0, index + 1 };
    }

    components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, s);
    components_type_info previous_info = TAGGED_UNION_GET_UNCHECKED(
        COMPONENTS_ALL_ID, 
        *LIST_GET(components_search_group, &d->components_search_groups, index)
    );
    if (info.component_count <= previous_info.component_count) {
        size_t index = previous_info.component_ids - (component_id *)d->component_ids.elements;
        if (LIST_GET(component_id, &d->component_ids, index) != previous_info.component_ids) {
            assert(false && "unreachable: failed component ids index computation");
            exit(EXIT_FAILURE);
        }

        TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, s).component_ids = LIST_SET_RANGE(
            component_id,
            &d->component_ids,
            (previous_info.component_ids - (component_id *)d->component_ids.elements),
            info.component_ids,
            info.component_count
        );
    } else {
        TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, s).component_ids = LIST_ADD_RANGE(
            component_id,
            &d->component_ids,
            a,
            info.component_ids,
            info.component_count
        );
    }

    LIST_SET(components_search_group, &d->components_search_groups, index, &s);
    return (components_search_group_range){ 0, index + 1 };
}

components_search_group_range dynamic_world_system_set_range(dynamic_world_system *d, arena *a, components_search_groups s, size_t index) {
    assert(index <= LIST_COUNT(components_search_group, &d->components_search_groups) && "index out of bounds");
    if (index + s.group_count >= LIST_COUNT(components_search_group, &d->components_search_groups) - 1) {
        size_t missing = index + s.group_count - LIST_COUNT(components_search_group, &d->components_search_groups) - 1;
        LIST_APPEND_EMPTY(components_search_group, &d->components_search_groups, a, missing);
    }

    for (size_t i = 0; i < s.group_count; ++i) {
        components_search_group group = s.groups[i];
        components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, group);
        components_type_info previous_info = TAGGED_UNION_GET_UNCHECKED(
            COMPONENTS_ALL_ID, 
            *LIST_GET(components_search_group, &d->components_search_groups, index)
        );
        if (info.component_count <= previous_info.component_count) {
            size_t index = previous_info.component_ids - (component_id *)d->component_ids.elements;
            if (LIST_GET(component_id, &d->component_ids, index) != previous_info.component_ids) {
                assert(false && "unreachable: failed component ids index computation");
                exit(EXIT_FAILURE);
            }

            TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, group).component_ids = LIST_SET_RANGE(
                component_id,
                &d->component_ids,
                (previous_info.component_ids - (component_id *)d->component_ids.elements),
                info.component_ids,
                info.component_count
            );
        } else {
            TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, group).component_ids = LIST_ADD_RANGE(
                component_id,
                &d->component_ids,
                a,
                info.component_ids,
                info.component_count
            );
        }
        LIST_SET(components_search_group, &d->components_search_groups, index + i, &group);
    }

    return (components_search_group_range){ 0, index + s.group_count };
}

world_system dynamic_world_system_get(const dynamic_world_system *d) {
    return world_system_create(components_search_groups_create(
        d->components_search_groups.elements,
        LIST_COUNT(components_search_group, &d->components_search_groups)
    ));
}

world_system dynamic_world_system_get_range(const dynamic_world_system *d, components_search_group_range r) {
    return world_system_create(components_search_groups_create(
        (components_search_group *)d->components_search_groups.elements + r.start,
        exclusive_range_length(r)
    ));
}

dynamic_world_system dynamic_world_system_clone(const dynamic_world_system *d, arena *a) {
    return dynamic_world_system_create_from(dynamic_world_system_get(d).search_groups, a);
}

#endif