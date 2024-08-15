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
        VIEW_STRUCT(entity_id) VIEW(entity_id); \
        MAP(_QUERY_RESULT_FIELD, __VA_ARGS__) \
    }

#define QUERY_RESULT_IMPLEMENT(...) \
    MAP(VIEW_IMPLEMENT, __VA_ARGS__)

void *query_run(const query q, const world *w, arena *query_arena) {
    size_t entity_count = world_entity_count(w);
    size_t initial_capacity = entity_count / 2 + 1;

    list *entity_results = (list *)arena_alloc(query_arena, sizeof(list) * (q.component_count + 1));
    *entity_results = list_create(query_arena, sizeof(entity_id) * (initial_capacity));

    list *component_results = entity_results + 1; 
    for (size_t i = 0; i < q.component_count; i++) {
        component_results[i] = list_create(query_arena, sizeof(void *) * (initial_capacity));
    }

    for (size_t i = 0; i < entity_count; i++) {
        entity e = *world_get_entity(w, i);
        if (((e.components & q.mask.component_mask) == q.mask.component_mask)
            && ((e.tags & q.mask.tag_mask) == q.mask.tag_mask)) {
            list_add(entity_results, query_arena, &e.id, sizeof(entity_id));
            
            for (size_t j = 0; j < q.component_count; j++) {
                void *component = world_components_get_component(
                    &w->components,
                    q.component_ids[j],
                    e.id,
                    w->components.component_sizes[q.component_ids[j]]
                );
                if (component == NULL) {
                    assert(0 && "unreachable: component not found");
                    exit(EXIT_FAILURE);
                }
                list_add(&component_results[j], query_arena, &component, sizeof(void *));
            }
        }
    }

    VIEW_STRUCT_INDIRECT(void, *) *component_views_result;
    VIEW_STRUCT(entity_id) *entity_view_result =
        arena_alloc(query_arena, sizeof(struct VIEW(entity_id)) + sizeof(struct VIEW(void)) * q.component_count);
    *entity_view_result = (struct VIEW(entity_id)) {
        .count = list_count_of_size(entity_results, sizeof(entity_id)),
        .elements = entity_results->elements,
    };

    component_views_result = (struct VIEW(void) *)(entity_view_result + 1);
    for (size_t i = 0; i < q.component_count; i++) {
        component_views_result[i] = (struct VIEW(void)) {
            .count = list_count_of_size(&component_results[i], sizeof(void *)),
            .elements = component_results[i].elements,
        };
    }
    return entity_view_result;
}
#define QUERY_RUN(query0, world_ref, arena_ref, ...) \
    (QUERY_RESULT_STRUCT(__VA_ARGS__) *)query_run(query0, world_ref, arena_ref)

#endif