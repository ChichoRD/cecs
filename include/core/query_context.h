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
}

void query_context_free(query_context *qc) {
    arena_free(&qc->query_arena);
}

typedef enum query_search_result {
    QUERY_SEARCH_RESULT_NOT_FOUND,
    QUERY_SEARCH_RESULT_FOUND,
    QUERY_SEARCH_RESULT_FOUND_SUBMASK,
} query_search_result;

typedef union query_mask_values {
    query_mask mask;
    uint32_t values[(sizeof(component_mask) + sizeof(tag_mask)) / sizeof(uint32_t)];
} query_mask_values;
#define QUERY_MASK_LENGTH (sizeof(query_mask_values) / sizeof(uint32_t))

static inline int32_t query_mask_values_compare(query_mask_values a, query_mask_values b) {
	for (size_t i = 0; i < QUERY_MASK_LENGTH; i++) {
		if (a.values[i] != b.values[i]) {
			return a.values[i] - b.values[i];
		}
	}
	return 0;
}

static size_t binary_search_operation_of_rank(const list* query_operations, query_mask_values values, size_t rank, size_t subrank) {
    size_t start = 0;
    size_t end = LIST_COUNT(query_operation, query_operations);
    size_t mid = 0;

    while (start < end) {
        mid = (start + end) / 2;
        query_operation* op = LIST_GET(query_operation, query_operations, mid);
        query_mask_values op_qmv = { .mask = op->query };
        for (size_t i = 0; i < QUERY_MASK_LENGTH; i++)
            op_qmv.values[i] <<= (rank - subrank);

        int32_t cmp = query_mask_values_compare(values, op_qmv);
        if (cmp < 0) {
			end = mid;
		} else if (cmp > 0) {
			start = mid + 1;
		} else {
			return mid;
		}
    }

    return start;
}

static query_search_result interpret_operation_index_of_rank(const list *query_operations, query_mask_values values, size_t operation_index, size_t rank, size_t subrank) {
    if (rank == subrank) {
        if (operation_index >= LIST_COUNT(query_operation, query_operations)) {
			return QUERY_SEARCH_RESULT_NOT_FOUND;
        }
        else {
            query_operation* op = LIST_GET(query_operation, query_operations, operation_index);
            query_mask_values op_qmv = { .mask = op->query };
            return query_mask_values_compare(values, op_qmv) == 0
                ? QUERY_SEARCH_RESULT_FOUND
                : QUERY_SEARCH_RESULT_NOT_FOUND;
        }
    } else if (operation_index < LIST_COUNT(query_operation, query_operations)) {
        query_operation* op = LIST_GET(query_operation, query_operations, operation_index);
        query_mask_values op_qmv = { .mask = op->query };
        bool submask = true;
        for (size_t j = 0; j < QUERY_MASK_LENGTH; j++) {
            if ((values.values[j] & op_qmv.values[j]) != op_qmv.values[j]) {
                submask = false;
                break;
            }
        }
        if (submask) {
            return QUERY_SEARCH_RESULT_FOUND_SUBMASK;
        }
    }

    return QUERY_SEARCH_RESULT_NOT_FOUND;
}

static query_search_result query_context_search_query(query_context *qc, const query q, list **out_query_operations, size_t *out_query_operation_index, query_operation **out_submask_operation) {
    *out_query_operations = NULL;
    *out_query_operation_index = 0;
    *out_submask_operation = NULL;

    uint32_t tag_count = __popcnt(q.mask.tag_mask);
    size_t rank = tag_count + q.component_count;

    query_mask_values qmv = { .mask = q.mask };
    query_search_result result = QUERY_SEARCH_RESULT_NOT_FOUND;

    for (int32_t i = rank; i >= 0; i--) {
        list *query_operations = &qc->queries_operations[i];    
        size_t index = binary_search_operation_of_rank(query_operations, qmv, rank, i);

        switch (interpret_operation_index_of_rank(query_operations, qmv, index, rank, i))
        {
            case QUERY_SEARCH_RESULT_NOT_FOUND: {
                if (*out_query_operations == NULL)
                    *out_query_operations = query_operations;
				*out_query_operation_index = index;
                *out_submask_operation = NULL;
				result = QUERY_SEARCH_RESULT_NOT_FOUND;
				break;
            }
            case QUERY_SEARCH_RESULT_FOUND: {
                if (*out_query_operations == NULL)
                    *out_query_operations = query_operations;
                *out_query_operation_index = index;
                *out_submask_operation = NULL;
				return QUERY_SEARCH_RESULT_FOUND;
			}
            case QUERY_SEARCH_RESULT_FOUND_SUBMASK: {
                assert(*out_query_operations != NULL && "out_query_operations is NULL, but interpret_operation_index_of_rank returned QUERY_SEARCH_RESULT_FOUND_SUBMASK");
                *out_query_operation_index = index;
                *out_submask_operation = LIST_GET(query_operation, query_operations, index);
                return QUERY_SEARCH_RESULT_FOUND_SUBMASK;
            }
            default: {
                assert(0 && "unreachable: interpret_operation_index_of_rank returned invalid value");
                exit(EXIT_FAILURE);
            }
        }
    }
    
    return result;
}

static void *query_context_run_query_on_operation(query_context *qc, const query q, const world *w, const query_operation operation) {
    size_t operation_component_count = __popcnt(operation.query.component_mask);
    QUERY_RESULT_STRUCT(void) *operation_views = (struct QUERY_RESULT(void) *)operation.result;
    size_t operation_size_of_partial_view = 
        sizeof(entity_id) + padding_between(entity_id, void *)
        + sizeof(void *) * (operation_component_count)
        + padding_between(void *, size_t) + sizeof(size_t);

    size_t size_of_partial_view =
        sizeof(entity_id) + padding_between(entity_id, void *)
        + sizeof(void *) * (q.component_count)
        + padding_between(void *, size_t) + sizeof(size_t);
    list *views_result = arena_alloc(&qc->query_arena, sizeof(list));
    *views_result = list_create(
        &qc->query_arena,
        size_of_partial_view);

    size_t total_count = 0;
    size_t contiguous_count = 0;
    for (size_t i = 0; i < operation_views->view_count; i++) {
        QUERY_VIEW_STRUCT(void) *view =
            ((uint8_t *)operation_views->query_views) + operation_size_of_partial_view * i;
        size_t view_count = *(((size_t *)(((uint8_t *)view) + operation_size_of_partial_view)) - 1);
        entity_id entity_id_view_start = view->query_elements.entity_id_view_start;
        
        for (size_t j = 0; j < view_count; j++) {
            entity *e = world_get_entity(w, entity_id_view_start + j);
            if (entity_classifies_for_query(e, q.mask)) {
                if (contiguous_count <= 0) {
                    list_add(views_result, &qc->query_arena, &e->id, sizeof(entity_id));
                    list_add(
                        views_result,
                        &qc->query_arena,
                        ((uint8_t[padding_between(entity_id, void *) + 1]){0}),
                        padding_between(entity_id, void *)
                    );
                    for (size_t k = 0; k < q.component_count; k++) {
                        void *component = world_get_component(
                            w,
                            q.component_ids[k],
                            e->id,
                            w->components.component_sizes[q.component_ids[k]]
                        );
                        if (component == NULL) {
                            assert(0 && "unreachable: component not found");
                            exit(EXIT_FAILURE);
                        }
                        list_add(views_result, &qc->query_arena, &component, sizeof(void *));
                    }
                }
                ++contiguous_count;
                if (i + 1 >= view_count) {
                    list_add(
                        views_result,
                        &qc->query_arena,
                        ((uint8_t[padding_between(void *, size_t) + 1]){0}),
                        padding_between(void *, size_t)
                    );
                    list_add(views_result, &qc->query_arena, &contiguous_count, sizeof(size_t));
                    total_count += contiguous_count;
                    contiguous_count = 0;
                }
            } else if ((contiguous_count > 0) || (j + 1 >= view_count)) {
                list_add(
                    views_result,
                    &qc->query_arena,
                    ((uint8_t[padding_between(void *, size_t) + 1]){0}),
                    padding_between(void *, size_t)
                );
                list_add(views_result, &qc->query_arena, &contiguous_count, sizeof(size_t));
                total_count += contiguous_count;
                contiguous_count = 0;
            }
        }
    }

    struct QUERY_RESULT(void) *result = (struct QUERY_RESULT(void) *)arena_alloc(&qc->query_arena, sizeof(struct QUERY_RESULT(void)));
    *result = (struct QUERY_RESULT(void)) {
        .query_views = views_result->elements,
        .view_count = list_count_of_size(views_result, size_of_partial_view),
        .found_entities_count = total_count,
    };
    return result;
}

void *query_context_run_query(query_context *qc, const query q, const world *w) {
    list *query_operations = NULL;
    size_t query_operation_index = 0;
    query_operation *submask_operation = NULL;
    query_search_result search_result = query_context_search_query(qc, q, &query_operations, &query_operation_index, &submask_operation);
    switch (search_result)
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