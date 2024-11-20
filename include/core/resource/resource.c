#include "resource.h"

resource_id resource_id_count = 0;

world_resources world_resources_create(size_t resource_capactity, size_t resource_default_size) {
    world_resources wr;
    wr.resources_arena = cecs_arena_create_with_capacity(resource_capactity * (resource_default_size + sizeof(resource_handle)));
    wr.resource_handles = cecs_displaced_set_create_with_capacity(&wr.resources_arena, sizeof(resource_handle) * resource_capactity);
    wr.discard = (resource_discard){ 0 };
    return wr;
}

void world_resources_free(world_resources* wr) {
    cecs_arena_free(&wr->resources_arena);
    wr->resource_handles = (cecs_displaced_set){ 0 };
    wr->discard = (resource_discard){ 0 };
}

bool world_resources_has_resource(const world_resources* wr, resource_id id) {
    return cecs_displaced_set_contains(&wr->resource_handles, (size_t)id, &(resource_handle){0}, sizeof(resource_handle));
}

resource_handle world_resources_set_resource(world_resources* wr, resource_id id, void* resource, size_t size) {
    if (size > wr->discard.size) {
        wr->discard.handle = cecs_arena_realloc(&wr->resources_arena, wr->discard.handle, wr->discard.size, size);
        wr->discard.size = size;
    }

    resource_handle handle = world_resources_has_resource(wr, id)
        ? *CECS_DISPLACED_SET_GET(resource_handle, &wr->resource_handles, (size_t)id)
        : cecs_arena_alloc(&wr->resources_arena, size);
    memcpy(handle, resource, size);
    return *CECS_DISPLACED_SET_SET(
        resource_handle,
        &wr->resource_handles,
        &wr->resources_arena,
        (size_t)id,
        &handle
    );
}

resource_handle world_resources_get_resource(const world_resources* wr, resource_id id) {
    assert(
        cecs_displaced_set_contains(&wr->resource_handles, (size_t)id, &(resource_handle){0}, sizeof(resource_handle))
        && "Resource ID does not correspond to an existing resource"
    );
    return *CECS_DISPLACED_SET_GET(resource_handle, &wr->resource_handles, (size_t)id);
}

bool world_resources_remove_resource(world_resources* wr, resource_id id) {
    return cecs_displaced_set_remove(&wr->resource_handles, (size_t)id, sizeof(resource_handle), &(resource_handle){0});
}

bool world_resources_remove_resource_out(world_resources* wr, resource_id id, resource_handle out_resource, size_t size) {
    assert(out_resource != NULL && "out_resource must not be NULL, use: world_resources_remove_resource");
    resource_handle removed_handle;
    if (cecs_displaced_set_remove_out(&wr->resource_handles, (size_t)id, &removed_handle, size, &(resource_handle){0})) {
        memcpy(out_resource, removed_handle, size);
        return true;
    }
    else {
        memset(out_resource, 0, size);
        return false;
    }
}
