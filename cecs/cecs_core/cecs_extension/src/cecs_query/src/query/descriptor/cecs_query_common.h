#ifndef CECS_QUERY_COMMON_H
#define CECS_QUERY_COMMON_H

#include <stdint.h>
#include <world/cecs_view.h>

typedef enum cecs_query_access_type {
    cecs_query_access_type_immutable = 0,
    cecs_query_access_type_mut,
    cecs_query_access_type_mut_alloc,
} cecs_query_access_type;
typedef uint8_t cecs_query_access_value;


typedef enum cecs_query_match_type {
    cecs_query_match_type_all = 0,
    cecs_query_match_type_any,
    cecs_query_match_type_none_of,
    // [...]
} cecs_query_match_type;
typedef uint8_t cecs_query_match_value;


// TODO: consider if 'flattened' query result is better instead of mimicing the descriptor structure in the result
typedef struct cecs_query_result {
    cecs_view *view_buffer;
    cecs_view_mut *view_mut_buffer;
    cecs_view_alloc *view_alloc_buffer;
} cecs_query_result;

inline cecs_query_result cecs_query_result_create(
    cecs_view *const view_buffer,
    cecs_view_mut *const view_mut_buffer,
    cecs_view_alloc *const view_alloc_buffer
) {
    cecs_query_result result = {
        .view_buffer = view_buffer,
        .view_mut_buffer = view_mut_buffer,
        .view_alloc_buffer = view_alloc_buffer,
    };
    return result;
}

#endif
