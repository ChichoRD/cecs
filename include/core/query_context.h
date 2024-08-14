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
} query_context;


query_context query_context_create(arena *query_arena) {
    query_context qc;
    for (size_t i = 0; i < QUERY_VARIANTS_COUNT; i++) {
        qc.queries_operations[i] = LIST_CREATE(query_operation, query_arena, 1);
    }
    return qc;
}

typedef enum query_search_result {
    QUERY_SEARCH_RESULT_NOT_FOUND,
    QUERY_SEARCH_RESULT_FOUND,
    QUERY_SEARCH_RESULT_FOUND_SUBMASK,
} query_search_result;

query_search_result query_context_search_query(query_context *qc, const query q, list **out_query_operations, size_t *out_query_operation_index, query_operation **out_submask_operation) {
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
    printf("tag_count: %d, rank: %zu\n", tag_count, rank);

    query_mask_values qmv = { .mask = q.mask };
    query_search_result result = QUERY_SEARCH_RESULT_NOT_FOUND;

    for (int32_t i = rank; i >= 0; i--) {
        printf("i: %zu\n", i);
        list *query_operations = &qc->queries_operations[i];
        
        size_t start = 0;
        size_t end = LIST_COUNT(query_operation, *query_operations);
        size_t mid = 0;
        printf("query_operations count: %d, capacity: %d\n", query_operations->count, query_operations->capacity);

        while (start < end) {
            mid = (start + end) / 2;
            printf("start: %zu, mid: %zu, end: %zu\n", start, mid, end);
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

        if (rank == i) {
            if (mid >= LIST_COUNT(query_operation, *query_operations)) {
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
        } else if (mid < LIST_COUNT(query_operation, *query_operations)) {
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
                *out_query_operations = query_operations;
                *out_query_operation_index = mid;
                *out_submask_operation = op->result;
                return QUERY_SEARCH_RESULT_FOUND_SUBMASK;
            }
        }
    }
    
    return result;
}

void *query_context_run_query_on_operation(const query q, const world *w, arena *query_arena, const query_operation operation) {
    size_t query_results_count = q.component_count + 1;
    VIEW_STRUCT(entity_id) *operation_views = (struct VIEW(entity_id) *)operation.result;
    size_t enitity_count = operation_views->count;

    list *results = arena_alloc(query_arena, sizeof(list) * query_results_count);
    results[0] = list_create(query_arena, sizeof(entity_id) * enitity_count);
    for (size_t i = 0; i < query_results_count; i++) {
        results[i] = list_create(query_arena, sizeof(intptr_t) * enitity_count);
    }
    
    for (size_t i = 0; i < enitity_count; i++) {
        entity e = *world_get_entity(w, operation_views->elements[i]);
        if (((e.components & q.mask.component_mask) == q.mask.component_mask)
            && ((e.tags & q.mask.tag_mask) == q.mask.tag_mask)) {
            list_add(&results[0], query_arena, &e.id, sizeof(entity_id));

            for (size_t j = 0; j < q.component_count; j++) {
                void *component = world_components_get_component(
                    &w->components,
                    q.component_ids[j],
                    e.id,
                    w->components.component_sizes[q.component_ids[j]]
                );

                list_add(&results[j + 1], query_arena, &component, sizeof(intptr_t));
            }
        }
    }

    struct VIEW(entity_id) *entity_id_view;
    VIEW_STRUCT_INDIRECT(void, *) *component_views;
    entity_id_view = arena_alloc(query_arena, sizeof(struct VIEW(void)) * q.component_count + sizeof(struct VIEW(entity_id)));
    component_views = (struct VIEW(void) *)(entity_id_view + 1);

    entity_id_view->count = LIST_COUNT_OF_SIZE(results[0], sizeof(entity_id));
    entity_id_view->elements = results[0].elements;
    for (size_t i = 0; i < q.component_count; i++) {
        component_views[i].count = LIST_COUNT_OF_SIZE(results[i + 1], sizeof(intptr_t));
        component_views[i].elements = results[i + 1].elements;
    }
    return entity_id_view;
}

void *query_context_run_query(query_context *qc, const query q, const world *w, arena *query_arena) {
    list *query_operations = NULL;
    size_t query_operation_index = 0;
    query_operation *submask_operation = NULL;
    switch (query_context_search_query(qc, q, &query_operations, &query_operation_index, &submask_operation))
    {
        case QUERY_SEARCH_RESULT_NOT_FOUND: {
            query_operation op = {
                .query = q.mask,
                .result = query_run(q, w, query_arena)
            };
            LIST_INSERT(query_operation, query_operations, query_arena, query_operation_index, op);
            return op.result;
        }

        case QUERY_SEARCH_RESULT_FOUND: {
            query_operation *op = LIST_GET(query_operation, query_operations, query_operation_index);
            return op->result;
        }

        case QUERY_SEARCH_RESULT_FOUND_SUBMASK: {
            assert(submask_operation != NULL && "submask_operation is NULL");
            query_operation op = {
                .query = q.mask,
                .result = query_context_run_query_on_operation(q, w, query_arena, *submask_operation)
            };
            LIST_INSERT(query_operation, query_operations, query_arena, query_operation_index, op);
            return op.result;
        }   
        default: {
            assert(0 && "unreachable: query_context_search_query returned invalid value");
            exit(EXIT_FAILURE);
            //return NULL;
        }
    }
}
#define QUERY_CONTEXT_RUN_QUERY(query_context_ref, query0, world_ref, query_arena_ref, ...) \
    ((QUERY_RESULT_STRUCT(__VA_ARGS__) *)query_context_run_query(query_context_ref, query0, world_ref, query_arena_ref))

#endif