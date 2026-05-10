#ifndef CECS_QUERY_DESCRIPTOR_SHARED_H
#define CECS_QUERY_DESCRIPTOR_SHARED_H

#include "../../registry/cecs_component.h"
#include "cecs_query_common.h"

typedef struct cecs_query_descriptor_shared_set {
    cecs_component_type *components;
    size_t component_count;
    cecs_query_access_value access;
    // FIXME: we ignore this in the implementations!
    cecs_query_match_value match;
} cecs_query_descriptor_shared_set;

typedef struct cecs_query_descriptor_shared {
    cecs_query_descriptor_shared_set *sets;
    size_t set_count;
} cecs_query_descriptor_shared;


#endif
