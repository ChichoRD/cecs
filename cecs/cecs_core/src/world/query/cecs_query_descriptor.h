#ifndef CECS_QUERY_DESCRIPTOR_H
#define CECS_QUERY_DESCRIPTOR_H

#include "descriptor/cecs_query_descriptor_shared.h"
#include <stdint.h>

typedef enum cecs_query_descriptor_type {
    // cecs_query_descriptor_type_none = 0,
    cecs_query_descriptor_type_shared = 0,
    cecs_query_descriptor_type_exclusive, // grouped
} cecs_query_descriptor_type;
typedef uint8_t cecs_query_descriptor_value;


typedef union cecs_internal_query_descriptor {
    cecs_query_descriptor_shared shared;
    // cecs_query_descriptor_exclusive exclusive;
} cecs_internal_query_descriptor;
typedef struct cecs_query_descriptor {
    cecs_internal_query_descriptor descriptor;
    cecs_query_descriptor_value type;
} cecs_query_descriptor;

#endif
