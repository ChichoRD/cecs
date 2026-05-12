#ifndef CECS_QUERY_DESCRIPTOR_H
#define CECS_QUERY_DESCRIPTOR_H

#include "descriptor/cecs_query_descriptor_shared.h"
#include <stdint.h>

typedef enum cecs_query_descriptor_type {
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


inline cecs_query_descriptor cecs_query_descriptor_create_shared(const cecs_query_descriptor_shared descriptor) {
    cecs_query_descriptor query_descriptor = {
        .descriptor.shared = descriptor,
        .type = cecs_query_descriptor_type_shared,
    };
    return query_descriptor;
}

#endif
