#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdint.h>
#include <string.h>
#include "../../containers/arena.h"
#include "../../containers/displaced_set.h"
#include "../../types/type_id.h"
#include "../../types/macro_utils.h"

typedef uint64_t resource_id;
static resource_id resource_id_count = 0;

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
    arena resources_arena;
    displaced_set resource_handles;
    resource_discard discard;
} world_resources;

world_resources world_resources_create(size_t resource_capactity, size_t resource_default_size) {
    world_resources wr;
    wr.resources_arena = arena_create_with_capacity(resource_capactity * (resource_default_size + sizeof(resource_handle)));
    wr.resource_handles = displaced_set_create_with_capacity(&wr.resources_arena, sizeof(resource_handle) * resource_capactity);
    wr.discard = (resource_discard){0};
    return wr;
}

void world_resources_free(world_resources *wr) {
    arena_free(&wr->resources_arena);
    wr->resource_handles = (displaced_set){0};
    wr->discard = (resource_discard){0};
}

bool world_resources_has_resource(const world_resources* wr, resource_id id) {
    return displaced_set_contains(&wr->resource_handles, id, &(resource_handle){0}, sizeof(resource_handle));
}

resource_handle world_resources_set_resource(world_resources *wr, resource_id id, void *resource, size_t size) {
    if (size > wr->discard.handle) {
        wr->discard.handle = arena_realloc(&wr->resources_arena, wr->discard.handle, wr->discard.size, size);
        wr->discard.size = size;
    }

    resource_handle handle = world_resources_has_resource(wr, id)
        ? *DISPLACED_SET_GET(resource_handle, &wr->resource_handles, id)
        : arena_alloc(&wr->resources_arena, size);
    memcpy(handle, resource, size);
    return *DISPLACED_SET_SET(
        resource_handle,
        &wr->resource_handles,
        &wr->resources_arena,
        id,
        &handle
    );
}

resource_handle world_resources_get_resource(const world_resources *wr, resource_id id) {
    assert(
        displaced_set_contains(&wr->resource_handles, id, &(resource_handle){0}, sizeof(resource_handle))
        && "Resource ID does not correspond to an existing resource"
    );
    return *DISPLACED_SET_GET(resource_handle, &wr->resource_handles, id);
}

bool world_resources_remove_resource(world_resources *wr, resource_id id) {
    return displaced_set_remove(&wr->resource_handles, id, sizeof(resource_handle), &(resource_handle){0});
}

bool world_resources_remove_resource_out(world_resources *wr, resource_id id, resource_handle out_resource, size_t size) {
    assert(out_resource != NULL && "out_resource must not be NULL, use: world_resources_remove_resource");
    resource_handle removed_handle;
    if (displaced_set_remove_out(&wr->resource_handles, id, &removed_handle, size, &(resource_handle){0})) {
        memcpy(out_resource, removed_handle, size);
        return true;
    } else {
        memset(out_resource, 0, size);
        return false;
    }
}

#endif