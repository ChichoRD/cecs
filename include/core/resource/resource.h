#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdint.h>
#include <string.h>
#include "../../containers/cecs_arena.h"
#include "../../containers/displaced_set.h"
#include "../../types/type_id.h"
#include "../../types/macro_utils.h"

typedef uint64_t resource_id;
extern resource_id resource_id_count;

#define RESOURCE(type) type##_resource
#define _RESOURCE_IMPLEMENT(resource) TYPE_ID_IMPLEMENT_COUNTER(resource, resource_id_count)
#define RESOURCE_IMPLEMENT(type) _RESOURCE_IMPLEMENT(RESOURCE(type))

#define RESOURCE_ID(type) ((resource_id)TYPE_ID(RESOURCE(type)))
#define RESOURCE_ID_ARRAY(...) ((resource_id[]){ MAP(RESOURCE_ID, COMMA, __VA_ARGS__) })

typedef void *resource_handle;
typedef struct resource_discard {
    resource_handle handle;
    size_t size;
} resource_discard;

typedef struct world_resources {
    cecs_arena resources_arena;
    displaced_set resource_handles;
    resource_discard discard;
} world_resources;

world_resources world_resources_create(size_t resource_capactity, size_t resource_default_size);

void world_resources_free(world_resources *wr);

bool world_resources_has_resource(const world_resources* wr, resource_id id);

resource_handle world_resources_set_resource(world_resources *wr, resource_id id, void *resource, size_t size);

resource_handle world_resources_get_resource(const world_resources *wr, resource_id id);

bool world_resources_remove_resource(world_resources *wr, resource_id id);

bool world_resources_remove_resource_out(world_resources *wr, resource_id id, resource_handle out_resource, size_t size);

#endif