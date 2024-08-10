#ifndef QUERY_H
#define QUERY_H

#include <stdint.h>
#include "entity.h"
#include "world.h"
#include "arena.h"
#include "list.h"
#include "view.h"

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
    )

#define _PREPEND_UNDERSOCRE(x) _##x
#define QUERY_RESULT(...) CAT(query_result, MAP_LIST(_PREPEND_UNDERSOCRE, __VA_ARGS__))

#define _QUERY_RESULT_FIELD(type) VIEW_STRUCT(type, *) VIEW(type);
#define QUERY_RESULT_STRUCT(...) \
    struct QUERY_RESULT(__VA_ARGS__) { \
        MAP(_QUERY_RESULT_FIELD, __VA_ARGS__) \
    }

#define QUERY_RESULT_IMPLEMENT(...) \
    MAP(VIEW_IMPLEMENT, __VA_ARGS__)

void *query_run(query q, const world *w, arena *query_arena) {
    list *results = arena_alloc(query_arena, sizeof(list) * q.component_count);
    for (size_t i = 0; i < q.component_count; i++) {
        results[i] = list_create(query_arena, sizeof(intptr_t) * WORLD_ENTITIES_COUNT(w->entities));
    }

    for (size_t i = 0; i < WORLD_ENTITIES_COUNT(w->entities); i++) {
        entity e = WORLD_ENTITIES_GET(w->entities, i);
        if ((e.mask & q.mask) == q.mask) {
            for (size_t j = 0; j < q.component_count; j++) {
                void *component = world_components_get_component(
                    &w->components,
                    q.component_ids[j],
                    e.id,
                    w->components.component_sizes[q.component_ids[j]]
                );

                list_add(&results[j], query_arena, &component, sizeof(intptr_t));
            }
        }
    }

    VIEW_STRUCT(void, *) *views = arena_alloc(query_arena, sizeof(struct VIEW(void)) * q.component_count);  
    for (size_t i = 0; i < q.component_count; i++) {
        views[i].count = LIST_COUNT_OF_SIZE(results[i], sizeof(intptr_t));
        views[i].elements = results[i].elements;
    }
    return views;
}
#define QUERY_RUN(query0, world0, arena0, ...) (QUERY_RESULT_STRUCT(__VA_ARGS__) *)query_run((query0), &(world0), &(arena0))

#endif