#ifndef CECS_RESOURCE_H
#define CECS_RESOURCE_H

#include <stdint.h>
#include <string.h>
#include "../../containers/cecs_arena.h"
#include "../../containers/cecs_displaced_set.h"
#include "../../types/cecs_type_id.h"
#include "../../types/cecs_macro_utils.h"

typedef uint64_t cecs_resource_id;
extern cecs_resource_id cecs_resource_id_count;

#define CECS_RESOURCE(type) type##_resource
#define _CECS_RESOURCE_IMPLEMENT(resource) CECS_TYPE_ID_IMPLEMENT_COUNTER(resource, cecs_resource_id_count)
#define CECS_RESOURCE_IMPLEMENT(type) _CECS_RESOURCE_IMPLEMENT(CECS_RESOURCE(type))

#define CECS_RESOURCE_ID(type) ((cecs_resource_id)CECS_TYPE_ID(CECS_RESOURCE(type)))
#define CECS_RESOURCE_ID_ARRAY(...) ((cecs_resource_id[]){ CECS_MAP(CECS_RESOURCE_ID, COMMA, __VA_ARGS__) })

typedef void *cecs_resource_handle;
typedef struct cecs_resource_discard {
    cecs_resource_handle handle;
    size_t size;
} cecs_resource_discard;

typedef struct cecs_world_resources {
    cecs_arena resources_arena;
    cecs_displaced_set resource_handles;
    cecs_resource_discard discard;
} cecs_world_resources;

cecs_world_resources cecs_world_resources_create(size_t resource_capactity, size_t resource_default_size);

void cecs_world_resources_free(cecs_world_resources *wr);

bool cecs_world_resources_has_resource(const cecs_world_resources* wr, cecs_resource_id id);

cecs_resource_handle cecs_world_resources_set_resource(cecs_world_resources *wr, cecs_resource_id id, void *resource, size_t size);

cecs_resource_handle cecs_world_resources_get_resource(const cecs_world_resources *wr, cecs_resource_id id);

bool cecs_world_resources_remove_resource(cecs_world_resources *wr, cecs_resource_id id);

bool cecs_world_resources_remove_resource_out(cecs_world_resources *wr, cecs_resource_id id, cecs_resource_handle out_resource, size_t size);

#endif