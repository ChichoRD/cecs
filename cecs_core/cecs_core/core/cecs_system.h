#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>
#include "../containers/cecs_union.h"
#include "component/cecs_component_iterator.h"
#include "cecs_world.h"

typedef struct cecs_world_system {
    cecs_component_iterator_descriptor descriptor;
} cecs_world_system;

static inline cecs_world_system cecs_world_system_create(cecs_component_iterator_descriptor descriptor) {
    return (cecs_world_system){ 
        .descriptor = descriptor
    };
}
#define CECS_WORLD_SYSTEM_CREATE_GROUPED(...) cecs_world_system_create(CECS_COMPONENT_ITERATOR_DESCRIPTOR_CREATE_GROUPPED(__VA_ARGS__))
#define CECS_WORLD_SYSTEM_CREATE_GROUPED_FROM_IDS(...) cecs_world_system_create(CECS_COMPONENT_ITERATOR_DESCRIPTOR_CREATE_GROUPPED(__VA_ARGS__))

#define CECS_WORLD_SYSTEM_CREATE(access, search, ...) CECS_WORLD_SYSTEM_CREATE_GROUPED(CECS_COMPONENT_GROUP(access, search, __VA_ARGS__))
#define CECS_WORLD_SYSTEM_CREATE_FROM_IDS(access, search, ...) CECS_WORLD_SYSTEM_CREATE_GROUPED(CECS_COMPONENT_GROUP_FROM_IDS(access, search, __VA_ARGS__))

typedef size_t cecs_entity_count;
typedef CECS_UNION_STRUCT(
    cecs_system_predicate_data,
    cecs_none,
    cecs_none,
    double,
    delta_time,
    void *,
    user_data
) cecs_system_predicate_data;

static inline cecs_system_predicate_data cecs_system_predicate_data_create_none(void) {
    return (cecs_system_predicate_data)CECS_UNION_CREATE(cecs_none, cecs_system_predicate_data, CECS_NONE);
}
static inline cecs_system_predicate_data cecs_system_predicate_data_create_delta_time(double delta_time_seconds) {
    return (cecs_system_predicate_data)CECS_UNION_CREATE(delta_time, cecs_system_predicate_data, delta_time_seconds);
}
static inline cecs_system_predicate_data cecs_system_predicate_data_create_user_data(void *data) {
    return (cecs_system_predicate_data)CECS_UNION_CREATE(user_data, cecs_system_predicate_data, data);
}

static inline double cecs_system_predicate_data_delta_time(cecs_system_predicate_data data) {
    return CECS_UNION_GET_UNCHECKED(delta_time, data);
}
static inline void *cecs_system_predicate_data_user_data(cecs_system_predicate_data data) {
    return CECS_UNION_GET_UNCHECKED(user_data, data);
}

typedef void *cecs_component_handles[];
typedef void cecs_system_predicate(const cecs_component_handles handles, cecs_component_id entity, cecs_world *world, const cecs_system_predicate_data data);
cecs_entity_count cecs_world_system_iter(
    const cecs_world_system s,
    cecs_world *w,
    cecs_arena *iteration_arena,
    cecs_component_handles handles,
    cecs_system_predicate_data data,
    cecs_system_predicate *const predicate
);
#define CECS_WORLD_SYSTEM_ITER(world_system0, world_ref, iteration_arena_ref, handles, predicate_data, predicate) \
    cecs_world_system_iter(world_system0, world_ref, iteration_arena_ref, handles, predicate_data, ((cecs_system_predicate *)predicate))


typedef struct cecs_system_predicates {
    cecs_system_predicate **predicates;
    size_t predicate_count;
} cecs_system_predicates;
cecs_system_predicates cecs_system_predicates_create(cecs_system_predicate **predicates, size_t predicate_count);
#define CECS_SYSTEM_PREDICATES_CREATE(...) \
    cecs_system_predicates_create( \
        (cecs_system_predicate *[]) { __VA_ARGS__ }, \
        sizeof((cecs_system_predicate *[]) { __VA_ARGS__ }) / sizeof(((cecs_system_predicate *[]) { __VA_ARGS__ })[0]) \
    )

cecs_entity_count cecs_world_system_iter_all(
    const cecs_world_system s,
    cecs_world *w,
    cecs_arena *iteration_arena,
    cecs_component_handles handles,
    cecs_system_predicate_data data,
    const cecs_system_predicates predicates
);
#define CECS_WORLD_SYSTEM_ITER_ALL(world_system0, world_ref, iteration_arena_ref, handles, predicate_data, ...) \
    cecs_world_system_iter_all(world_system0, world_ref, iteration_arena_ref, handles, predicate_data, CECS_SYSTEM_PREDICATES_CREATE(__VA_ARGS__))


typedef struct cecs_dynamic_world_system {
    cecs_dynamic_array components;
    cecs_dynamic_array component_groups;
} cecs_dynamic_world_system;

cecs_dynamic_world_system cecs_dynamic_world_system_create(void);
cecs_dynamic_world_system cecs_dynamic_world_system_create_from(cecs_arena *a, const cecs_component_iteration_group groups[const], const size_t group_count);

typedef cecs_exclusive_range cecs_component_iteration_group_range;
cecs_component_iteration_group_range cecs_dynamic_world_system_add_range(
    cecs_dynamic_world_system *d,
    cecs_arena *a,
    const cecs_component_iteration_group groups[const],
    const size_t group_count
);
cecs_component_iteration_group_range cecs_dynamic_world_system_set_or_extend_range(
    cecs_dynamic_world_system *d,
    cecs_arena *a,
    const size_t index,
    const cecs_component_iteration_group groups[const],
    const size_t group_count
);

cecs_world_system cecs_world_system_from_dynamic(const cecs_dynamic_world_system *d);
cecs_world_system cecs_world_system_from_dynamic_range(const cecs_dynamic_world_system *d, const cecs_component_iteration_group_range r);

cecs_dynamic_world_system cecs_dynamic_world_system_clone(const cecs_dynamic_world_system *d, cecs_arena *a);

#endif