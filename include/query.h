#ifndef QUERY_H
#define QUERY_H

#include <stdint.h>
#include "entity.h"
#include "world.h"
#include "arena.h"
#include "list.h"

typedef struct query {
    component_mask mask;
    component_id *component_ids;
    size_t component_count;
} query;

query query_create(component_mask mask, component_id *component_ids, size_t component_count) {
    query q;
    q.mask = mask;
    q.component_ids = component_ids;
    q.component_count = component_count;
    return q;
}
#define QUERY_CREATE(...) \
    query_create( \
        COMPONENTS_MASK(__VA_ARGS__), \
        COMPONENT_ID_ARRAY(__VA_ARGS__), \
        sizeof(COMPONENT_ID_ARRAY(__VA_ARGS__)) / sizeof(component_id) \
    ) \


// TODO: Cleanup

#define _QUERY_RESULT_FIELDS(type) \
    size_t type##_count; \
    type **type##_results;

#define _QUERY_RESULT_FIELD_TYPE(type) \
    struct type##_result { \
        _QUERY_RESULT_FIELDS(type) \
    }

#define _QUERY_RESULT_FIELD_TYPE_IDENTIFIER_PAIR(type) \
    _QUERY_RESULT_FIELD_TYPE(type) type##_view;

#define _PREPEND_UNDESCORE(x) _##x

#define QUERY_RESULT_TYPENAME(...) CAT(query_result, MAP_LIST(_PREPEND_UNDESCORE, __VA_ARGS__))
#define QUERY_RESULT_TYPE(...) struct QUERY_RESULT_TYPENAME(__VA_ARGS__)

#define QUERY_RESULT(...) \
    QUERY_RESULT_TYPE(__VA_ARGS__) { \
        MAP(_QUERY_RESULT_FIELD_TYPE_IDENTIFIER_PAIR, __VA_ARGS__) \
    }


void *query_run(query *q, world *w, arena *query_arena) {
    list *results = arena_alloc(query_arena, sizeof(list) * q->component_count);
    for (size_t i = 0; i < q->component_count; i++) {
        results[i] = list_create(query_arena, sizeof(intptr_t) * WORLD_ENTITIES_COUNT(w->entities));
    }

    WORLD_FOREACH_ENTITY(*w, entity0) {
        if ((entity0->mask & q->mask) == q->mask) {
            for (size_t i = 0; i < q->component_count; i++) {
                void *component = world_components_get_component(
                    &w->components,
                    q->component_ids[i],
                    entity0->id,
                    w->components.component_sizes[q->component_ids[i]]
                );

                list_add(&results[i], query_arena, &component, sizeof(intptr_t));
            }
        }
    }

    struct result {
        size_t count;
        void *elements;
    };

    struct result *results_array = arena_alloc(query_arena, sizeof(struct result) * q->component_count);
    for (size_t i = 0; i < q->component_count; i++) {
        results_array[i].count = LIST_COUNT_OF_SIZE(results[i], sizeof(intptr_t));
        results_array[i].elements = results[i].elements;
    }

    return results_array;
}
#define QUERY_RUN(query0, world0, arena0, ...) (QUERY_RESULT_TYPE(__VA_ARGS__) *)query_run(&(query0), &(world0), &(arena0))

#endif