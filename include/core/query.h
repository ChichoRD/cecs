#ifndef QUERY_H
#define QUERY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
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

inline bool entity_classifies_for_query(const entity *e, const query_mask mask) {
    return entity_has_components(e, mask.component_mask) && entity_has_tags(e, mask.tag_mask);
}

#define _PREPEND_UNDERSCORE(x) _##x
#define QUERY_VIEW_FIELD(type) type *type##_elements;

#define QUERY_VIEW_ELEMENTS query_elements
#define QUERY_VIEW_ELEMENTS_STRUCT(...) \
    struct QUERY_VIEW_ELEMENTS { \
        entity_id entity_id_view_start; \
        MAP(QUERY_VIEW_FIELD, __VA_ARGS__) \
    }

#define QUERY_VIEW(...) CAT(query_view, MAP_LIST(_PREPEND_UNDERSCORE, __VA_ARGS__))
#define QUERY_VIEW_STRUCT(...) \
    struct QUERY_VIEW(__VA_ARGS__) { \
        QUERY_VIEW_ELEMENTS_STRUCT(__VA_ARGS__) QUERY_VIEW_ELEMENTS; \
        size_t count; \
    }

#define QUERY_RESULT(...) CAT(query_result, MAP_LIST(_PREPEND_UNDERSCORE, __VA_ARGS__))
#define QUERY_RESULT_STRUCT(...) \
    struct QUERY_RESULT(__VA_ARGS__) { \
        QUERY_VIEW_STRUCT(__VA_ARGS__) *query_views; \
        size_t view_count; \
        size_t found_entities_count; \
    }

void *query_run(const query q, const world *w, arena *query_arena) {
    size_t entity_count = world_entity_count(w);
    size_t size_of_partial_view =
        sizeof(entity_id) + padding_between(entity_id, void *)
        + sizeof(void *) * (q.component_count)
        + padding_between(void *, size_t) + sizeof(size_t);
    list views_result = list_create(query_arena, size_of_partial_view);

    size_t total_count = 0;
    size_t contiguous_count = 0;
    for (size_t i = 0; i < entity_count; i++) {
        entity *e = world_get_entity(w, i);
        if (entity_classifies_for_query(e, q.mask)) {
            if (contiguous_count <= 0) {
                list_add(&views_result, query_arena, &e->id, sizeof(entity_id));
                list_add(
                    &views_result,
                    query_arena,
                    ((uint8_t[padding_between(entity_id, void *) + 1]){0}),
                    padding_between(entity_id, void *)
                );
                for (size_t j = 0; j < q.component_count; j++) {
                    void *component = world_get_component(
                        w,
                        q.component_ids[j],
                        e->id,
                        w->components.component_sizes[q.component_ids[j]]
                    );
                    if (component == NULL) {
                        assert(0 && "unreachable: component not found");
                        exit(EXIT_FAILURE);
                    }
                    list_add(&views_result, query_arena, &component, sizeof(void *));
                }
            }

            ++contiguous_count;
            if (i + 1 >= entity_count) {
                list_add(
                    &views_result,
                    query_arena,
                    ((uint8_t[padding_between(void *, size_t) + 1]){0}),
                    padding_between(void *, size_t)
                );
                list_add(&views_result, query_arena, &contiguous_count, sizeof(size_t));
                total_count += contiguous_count;
                contiguous_count = 0;
            }
        } else if ((contiguous_count > 0) || (i + 1 >= entity_count)) {
            list_add(
                &views_result,
                query_arena,
                ((uint8_t[padding_between(void *, size_t) + 1]){0}),
                padding_between(void *, size_t)
            );
            list_add(&views_result, query_arena, &contiguous_count, sizeof(size_t));
            total_count += contiguous_count;
            contiguous_count = 0;
        }
    }

    QUERY_RESULT_STRUCT(void) *result = (struct QUERY_RESULT(void) *)arena_alloc(query_arena, sizeof(struct QUERY_RESULT(void)));
    *result = (struct QUERY_RESULT(void)) {
        .query_views = views_result.elements,
        .view_count = list_count_of_size(&views_result, size_of_partial_view),
        .found_entities_count = total_count,
    };
    return result;
}
#define QUERY_RUN(query0, world_ref, arena_ref, ...) \
    (QUERY_RESULT_STRUCT(__VA_ARGS__) *)query_run(query0, world_ref, arena_ref)


typedef struct query_iterator {
    const QUERY_RESULT_STRUCT(void) *result;
    const size_t view_size;
    size_t view_index;
    size_t current_view_index;
} query_iterator;

query_iterator query_iterator_create(const void *query_result, const size_t view_size) {
    return (query_iterator) {
        .result = query_result,
        .view_size = view_size,
        .view_index = 0,
        .current_view_index = 0,
    };
}
#define QUERY_ITERATOR_CREATE(query_result_ref, ...) \
    query_iterator_create(query_result_ref, sizeof(QUERY_VIEW_STRUCT(__VA_ARGS__)))

void *query_iterator_current_count(const query_iterator *it, size_t *current_view_count) {
    assert(it->view_index < it->result->view_count);
    void *current_view = ((uint8_t *)it->result->query_views) + it->view_size * it->view_index;
    *current_view_count = *(((size_t *)(((uint8_t *)current_view) + it->view_size)) - 1);
    assert(it->current_view_index < current_view_count);
    return current_view;
}
#define QUERY_ITERATOR_CURRENT_COUNT(query_iterator_ref, ...) \
    ((QUERY_VIEW_STRUCT(__VA_ARGS__) *)query_iterator_current_count(query_iterator_ref, &((size_t){0})))

void *query_iterator_current(const query_iterator *it) {
    assert(it->view_index < it->result->view_count
        && "invalid iterator state, it may have reached the end of its query already; create a new iterator");
    void *current_view = ((uint8_t *)it->result->query_views) + it->view_size * it->view_index;
    size_t current_view_count = *(((size_t *)(((uint8_t *)current_view) + it->view_size)) - 1);
    assert(it->current_view_index < current_view_count
        && "invalid iterator state, it may have reached the end of its query already; create a new iterator");
    return current_view;
}
#define QUERY_ITERATOR_CURRENT(query_iterator_ref, ...) \
    ((QUERY_VIEW_ELEMENTS_STRUCT(__VA_ARGS__) *)query_iterator_current(query_iterator_ref))

void query_iterator_next(query_iterator *it) {
    size_t current_view_count = 0;
    void *current_view = query_iterator_current_count(it, &current_view_count);
    if (++it->current_view_index >= current_view_count) {
        ++it->view_index;
        it->current_view_index = 0;
    }
}

bool query_iterator_done(const query_iterator *it) {
    return it->view_index >= it->result->view_count;
}

#define QUERY_RUN_INTO_ITER(query0, world_ref, arena_ref, ...) \
    (QUERY_ITERATOR_CREATE(query_run(query0, world_ref, arena_ref), __VA_ARGS__))

#endif
