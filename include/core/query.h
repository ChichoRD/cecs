#ifndef QUERY_H
#define QUERY_H

#include <stdint.h>
#include "entity.h"
#include "world.h"
#include "arena.h"
#include "list.h"
#include "view.h"
#include "component.h"
#include "tag.h"

typedef struct query_mask {
    const component_mask component_mask;
    const tag_mask tag_mask;
} query_mask;

typedef struct query {
    const query_mask mask;
    const component_id *component_ids;
    const size_t component_count;
} query;

query query_create(const component_mask component_mask, const component_id *component_ids, const size_t component_count, const tag_mask tag_mask) {
    return (query) {
        .mask = (query_mask) {
            .component_mask = component_mask,
            .tag_mask = tag_mask,
        },
        .component_ids = component_ids,
        .component_count = component_count,
    };
}
#define QUERY_CREATE(with_components, with_tags) \
    query_create( \
        with_components, \
        with_tags \
    )

#define WITH_COMPONENTS(...) \
    COMPONENTS_MASK(__VA_ARGS__), \
    COMPONENT_ID_ARRAY(__VA_ARGS__), \
    sizeof(COMPONENT_ID_ARRAY(__VA_ARGS__)) / sizeof(component_id)
#define WITH_TAGS(...) TAGS_MASK(__VA_ARGS__)
#define WITHOUT_TAGS (0)


#define _PREPEND_UNDERSOCRE(x) _##x
#define QUERY_RESULT(...) CAT(query_result, MAP_LIST(_PREPEND_UNDERSOCRE, __VA_ARGS__))

#define _QUERY_RESULT_FIELD(type) VIEW_STRUCT_INDIRECT(type, *) VIEW(type);
#define QUERY_RESULT_STRUCT(...) \
    struct QUERY_RESULT(__VA_ARGS__) { \
        MAP(_QUERY_RESULT_FIELD, __VA_ARGS__) \
    }

#define QUERY_RESULT_IMPLEMENT(...) \
    MAP(VIEW_IMPLEMENT, __VA_ARGS__)

void *query_run(const query q, const world *w, arena *query_arena) {
    list *results = arena_alloc(query_arena, sizeof(list) * q.component_count);
    size_t enitity_count = world_entity_count(w);
    for (size_t i = 0; i < q.component_count; i++) {
        results[i] = list_create(query_arena, sizeof(intptr_t) * enitity_count);
    }

    for (size_t i = 0; i < enitity_count; i++) {
        entity e = *world_get_entity(w, i);
        if (((e.components & q.mask.component_mask) == q.mask.component_mask)
            && ((e.tags & q.mask.tag_mask) == q.mask.tag_mask)) {
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

    VIEW_STRUCT_INDIRECT(void, *) *views = arena_alloc(query_arena, sizeof(struct VIEW(void)) * q.component_count);  
    for (size_t i = 0; i < q.component_count; i++) {
        views[i].count = LIST_COUNT_OF_SIZE(results[i], sizeof(intptr_t));
        views[i].elements = results[i].elements;
    }
    return views;
}
#define QUERY_RUN(query0, world0, arena0, ...) (QUERY_RESULT_STRUCT(__VA_ARGS__) *)query_run((query0), &(world0), &(arena0))

#endif