#ifndef QUERY_CONTEXT_H
#define QUERY_CONTEXT_H

#include <intrin.h>
#include <stdint.h>
#include <stdbool.h>
#include "query.h"
#include "component.h"
#include "tag.h"
#include "list.h"

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

void *query_context_run_query(query_context *qc, const query q, const world *w, arena *query_arena) {
    uint32_t tag_count = __popcnt(q.mask.tag_mask);
    size_t rank = tag_count + q.component_count;

    typedef uint32_t query_mask_values[(sizeof(component_mask) + sizeof(tag_mask)) / sizeof(uint32_t)];
    #define QUERY_MASK_LENGTH (sizeof(query_mask_values) / sizeof(uint32_t))
    query_mask_values qmv = *(query_mask_values *)(&q.mask);
    void **result = arena_alloc(query_arena, sizeof(void *));

    for (size_t i = rank; i >= 0; i--) {
        list query_operations = qc->queries_operations[i];

        if ((i == rank)
            && (query_operations.count == 0)) {
            LIST_ADD(query_operation, &query_operations, query_arena, ((query_operation){
                .query = q.mask,
                .result = *result,
            }));
        }
        
        size_t start = 0;
        size_t end = LIST_COUNT(query_operation, query_operations);

        while (start < end) {
            size_t mid = (start + end) / 2;
            query_operation *op = LIST_GET(query_operation, &query_operations, mid);
            query_mask_values op_qmv = *(query_mask_values *)(&op->query);

            bool less = false;
            for (size_t i = 0; i < QUERY_MASK_LENGTH; i++) {
                if (qmv[i] < (op_qmv[i] << (rank - i))) {
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
            if (start == LIST_COUNT(query_operation, query_operations)) {
                LIST_ADD(query_operation, &query_operations, query_arena, ((query_operation){
                    .query = q.mask,
                    .result = *result,
                }));
            } else {
                query_operation *op = LIST_GET(query_operation, &query_operations, start);
                query_mask_values op_qmv = *(query_mask_values *)(&op->query);
                bool equal = true;
                for (size_t i = 0; i < QUERY_MASK_LENGTH; i++) {
                    if (qmv[i] != op_qmv[i]) {
                        equal = false;
                        break;
                    }
                }
                if (equal) {
                    return (*result = op->result);
                } else {
                    LIST_INSERT(query_operation, &query_operations, start, query_arena, ((query_operation){
                        .query = q.mask,
                        .result = *result,
                    }));
                }
            }
        } else if (start < LIST_COUNT(query_operation, query_operations)) {
            query_operation *op = LIST_GET(query_operation, &query_operations, start);
            query_mask_values op_qmv = *(query_mask_values *)(&op->query);
            bool submask = true;
            for (size_t i = 0; i < QUERY_MASK_LENGTH; i++) {
                if ((qmv[i] & op_qmv[i]) != op_qmv[i]) {
                    submask = false;
                    break;
                }
            }
            if (submask) {
                // TODO: implement submask search
            }
        }
    }

    return (*result = query_run(q, w, query_arena));
}
#define QUERY_CONTEXT_RUN_QUERY(query_context_ref, query0, world_ref, query_arena_ref, ...) \
    ((QUERY_RESULT_STRUCT(__VA_ARGS__) *)query_context_run_query(query_context_ref, query0, world_ref, query_arena_ref))

#endif