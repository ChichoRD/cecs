#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdint.h>
#include "map.h"
#include "type_id.h"

typedef uint32_t resource_id;
static resource_id resource_id_count = 0;

#define RESOURCE(type) type##_resource
#define RESOURCE_IMPLEMENT(type) TYPE_ID_IMPLEMENT_COUNTER(type##_resource, resource_id_count)
#define RESOURCE_DEFINE(type) \
    RESOURCE_IMPLEMENT(type) \
    typedef struct type##_resource

#define RESOURCE_ID(type) ((resource_id)TYPE_ID(type##_resource))
#define RESOURCE_ID_ARRAY(...) (resource_id[]){ MAP_LIST(RESOURCE_ID, __VA_ARGS__) }

#endif