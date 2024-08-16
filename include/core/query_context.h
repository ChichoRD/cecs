#ifndef QUERY_CONTEXT_H
#define QUERY_CONTEXT_H

#include <intrin.h>
#include <stdint.h>
#include <stdbool.h>
#include "query.h"
#include "component.h"
#include "tag.h"
#include "list.h"
#include "view.h"
#include "entity.h"

#define QUERY_VARIANTS_COUNT (COMPONENT_TYPE_COUNT + TAG_TYPE_COUNT)

typedef struct query_operation {
    const query_mask query;
    void *result;
} query_operation;

typedef struct query_context {
    list queries_operations[QUERY_VARIANTS_COUNT];
    arena query_arena;
} query_context;


query_context query_context_create() {
    query_context qc;
    qc.query_arena = arena_create();
    for (size_t i = 0; i < QUERY_VARIANTS_COUNT; i++) {
        qc.queries_operations[i] = LIST_CREATE(query_operation, &qc.query_arena, 1);
    }
    return qc;
}

void query_context_clear(query_context *qc) {
    for (size_t i = 0; i < QUERY_VARIANTS_COUNT; i++) {
        list_clear(&qc->queries_operations[i]);
    }
    arena_clear(&qc->query_arena);
}

void query_context_free(query_context *qc) {
    arena_free(&qc->query_arena);
}

typedef enum query_search_result {
    QUERY_SEARCH_RESULT_NOT_FOUND,
    QUERY_SEARCH_RESULT_FOUND,
    QUERY_SEARCH_RESULT_FOUND_SUBMASK,
} query_search_result;

static query_search_result query_context_search_query(query_context *qc, const query q, list **out_query_operations, size_t *out_query_operation_index, query_operation **out_submask_operation) {
    typedef union query_mask_values {
        query_mask mask;
        uint32_t values[(sizeof(component_mask) + sizeof(tag_mask)) / sizeof(uint32_t)];
    } query_mask_values;
    #define QUERY_MASK_LENGTH (sizeof(query_mask_values) / sizeof(uint32_t))

    *out_query_operations = NULL;
    *out_query_operation_index = 0;
    *out_submask_operation = NULL;

    uint32_t tag_count = __popcnt(q.mask.tag_mask);
    size_t rank = tag_count + q.component_count;

    query_mask_values qmv = { .mask = q.mask };
    query_search_result result = QUERY_SEARCH_RESULT_NOT_FOUND;

    for (int32_t i = rank; i >= 0; i--) {
        list *query_operations = &qc->queries_operations[i];
        
        size_t start = 0;
        size_t end = LIST_COUNT(query_operation, query_operations);
        size_t mid = 0;

        while (start < end) {
            mid = (start + end) / 2;
            query_operation *op = LIST_GET(query_operation, query_operations, mid);
            query_mask_values op_qmv = { .mask = op->query };

            bool less = false;
            for (size_t j = 0; j < QUERY_MASK_LENGTH; j++) {
                if (qmv.values[j] < (op_qmv.values[j] << (rank - i))) {
                    less = true;
                    break;
                }
            }

            if (less) {
                end = mid;
            } else {
                start = mid + 1;
            }
        }

        if (rank == (size_t)i) {
            if (mid >= LIST_COUNT(query_operation, query_operations)) {
                result = QUERY_SEARCH_RESULT_NOT_FOUND;
                *out_query_operations = query_operations;
                *out_query_operation_index = mid;
                *out_submask_operation = NULL;
            } else {
                query_operation *op = LIST_GET(query_operation, query_operations, mid);
                query_mask_values op_qmv = { .mask = op->query };
                bool equal = true;
                for (size_t j = 0; j < QUERY_MASK_LENGTH; j++) {
                    if (qmv.values[j] != op_qmv.values[j]) {
                        equal = false;
                        break;
                    }
                }
                if (equal) {
                    *out_query_operations = query_operations;
                    *out_query_operation_index = mid;
                    *out_submask_operation = op;
                    return QUERY_SEARCH_RESULT_FOUND;
                } else {
                    result = QUERY_SEARCH_RESULT_NOT_FOUND;
                    *out_query_operations = query_operations;
                    *out_query_operation_index = mid;
                    *out_submask_operation = NULL;
                }
            }
        } else if (mid < LIST_COUNT(query_operation, query_operations)) {
            query_operation *op = LIST_GET(query_operation, query_operations, mid);
            query_mask_values op_qmv = { .mask = op->query };
            bool submask = true;
            for (size_t j = 0; j < QUERY_MASK_LENGTH; j++) {
                if ((qmv.values[j] & op_qmv.values[j]) != op_qmv.values[j]) {
                    submask = false;
                    break;
                }
            }
            if (submask) {
                assert(*out_query_operations != NULL && "out_query_operations is NULL, but submask is true");
                *out_query_operation_index = mid;
                *out_submask_operation = op;
                return QUERY_SEARCH_RESULT_FOUND_SUBMASK;
            }
        }
    }
    
    return result;
}

static void *query_context_run_query_on_operation(query_context *qc, const query q, const world *w, const query_operation operation) {
    VIEW_STRUCT(entity_id) *operation_views = (struct VIEW(entity_id) *)operation.result;
    size_t entity_count = operation_views->count;
    size_t initial_capacity = entity_count / 2 + 1;

    list *entity_results = (list *)arena_alloc(&qc->query_arena, sizeof(list) * (q.component_count + 1));
    *entity_results = list_create(&qc->query_arena, sizeof(entity_id) * (initial_capacity));

    list *component_results = entity_results + 1; 
    for (size_t i = 0; i < q.component_count; i++) {
        component_results[i] = list_create(&qc->query_arena, sizeof(void *) * (initial_capacity));
    }
    
    for (size_t i = 0; i < entity_count; i++) {
        entity e = *world_get_entity(w, operation_views->elements[i]);
        if (((e.components & q.mask.component_mask) == q.mask.component_mask)
            && ((e.tags & q.mask.tag_mask) == q.mask.tag_mask)) {
            list_add(entity_results, &qc->query_arena, &e.id, sizeof(entity_id));

            for (size_t j = 0; j < q.component_count; j++) {
                void *component = world_components_get_component(
                    &w->components,
                    q.component_ids[j],
                    e.id,
                    w->components.component_sizes[q.component_ids[j]]
                );

                list_add(&component_results[j], &qc->query_arena, &component, sizeof(void *));
            }
        }
    }

    VIEW_STRUCT_INDIRECT(void, *) *component_views_result;
    struct VIEW(entity_id) *entity_view_result =
        arena_alloc(&qc->query_arena, sizeof(struct VIEW(entity_id)) + sizeof(struct VIEW(void)) * q.component_count);
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

void *query_context_run_query(query_context *qc, const query q, const world *w) {
    list *query_operations = NULL;
    size_t query_operation_index = 0;
    query_operation *submask_operation = NULL;
    switch (query_context_search_query(qc, q, &query_operations, &query_operation_index, &submask_operation))
    {
        case QUERY_SEARCH_RESULT_NOT_FOUND: {
            query_operation op = {
                .query = q.mask,
                .result = query_run(q, w, &qc->query_arena)
            };
            return LIST_INSERT(query_operation, query_operations, &qc->query_arena, query_operation_index, &op)->result;
        }

        case QUERY_SEARCH_RESULT_FOUND: {
            return LIST_GET(query_operation, query_operations, query_operation_index)->result;
        }

        case QUERY_SEARCH_RESULT_FOUND_SUBMASK: {
            assert(submask_operation != NULL && "submask_operation is NULL, but search result is QUERY_SEARCH_RESULT_FOUND_SUBMASK");
            query_operation op = {
                .query = q.mask,
                .result = query_context_run_query_on_operation(qc, q, w, *submask_operation)
            };
            return LIST_INSERT(query_operation, query_operations, &qc->query_arena, query_operation_index, &op)->result;
        }   

        default: {
            assert(0 && "unreachable: query_context_search_query returned invalid value");
            exit(EXIT_FAILURE);
            return NULL;
        }
    }
}
#define QUERY_CONTEXT_RUN_QUERY(query_context_ref, query0, world_ref, ...) \
    ((QUERY_RESULT_STRUCT(__VA_ARGS__) *)query_context_run_query(query_context_ref, query0, world_ref))

#endif