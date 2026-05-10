#ifndef CECS_QUERY_COMMON_H
#define CECS_QUERY_COMMON_H

#include <stdint.h>
#include "../../cecs_view.h"

typedef enum cecs_query_access_type {
    cecs_query_access_type_none = 0,
    cecs_query_access_type_immutable,
    cecs_query_access_type_mut,
    cecs_query_access_type_mut_alloc,
} cecs_query_access_type;
typedef uint8_t cecs_query_access_value;


typedef enum cecs_query_match_type {
    cecs_query_match_type_none = 0,
    cecs_query_match_type_all,
    cecs_query_match_type_any,
    cecs_query_match_type_none_of,
    // [...]
} cecs_query_match_type;
typedef uint8_t cecs_query_match_value;


typedef struct cecs_query_result {
    cecs_view *view_buffer;
    cecs_view_mut *view_mut_buffer;
    cecs_view_alloc *view_alloc_buffer;
} cecs_query_result;

#endif
