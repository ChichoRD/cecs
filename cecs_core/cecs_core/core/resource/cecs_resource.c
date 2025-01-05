#include <string.h>
#include <assert.h>

#include "cecs_resource.h"

static cecs_resource_id cecs_resource_id_count = 0;

cecs_resource_id *cecs_resource_id_counter(void) {
    return &cecs_resource_id_count;
}

cecs_world_resources cecs_world_resources_create(size_t resource_capactity, size_t resource_default_size)
{
    cecs_world_resources wr;
    wr.resources_arena = cecs_arena_create_with_capacity(resource_capactity * (resource_default_size + sizeof(cecs_resource_handle)));
    wr.resource_handles = cecs_displaced_set_create_with_capacity(&wr.resources_arena, sizeof(cecs_resource_handle) * resource_capactity);
    wr.discard = (cecs_resource_discard){ 0 };
    return wr;
}

void cecs_world_resources_free(cecs_world_resources* wr) {
    cecs_arena_free(&wr->resources_arena);
    wr->resource_handles = (cecs_displaced_set){ 0 };
    wr->discard = (cecs_resource_discard){ 0 };
}

bool cecs_world_resources_has_resource(const cecs_world_resources* wr, cecs_resource_id id) {
    return cecs_displaced_set_contains(&wr->resource_handles, (size_t)id, &(cecs_resource_handle){0}, sizeof(cecs_resource_handle));
}

cecs_resource_handle cecs_world_resources_set_resource(cecs_world_resources* wr, cecs_resource_id id, void* resource, size_t size) {
    if (size > wr->discard.size) {
        wr->discard.handle = cecs_arena_realloc(&wr->resources_arena, wr->discard.handle, wr->discard.size, size);
        wr->discard.size = size;
    }

    cecs_resource_handle handle = cecs_world_resources_has_resource(wr, id)
        ? *CECS_DISPLACED_SET_GET(cecs_resource_handle, &wr->resource_handles, (size_t)id)
        : cecs_arena_alloc(&wr->resources_arena, size);
    memcpy(handle, resource, size);
    return *CECS_DISPLACED_SET_SET(
        cecs_resource_handle,
        &wr->resource_handles,
        &wr->resources_arena,
        (size_t)id,
        &handle
    );
}

cecs_resource_handle cecs_world_resources_get_resource(const cecs_world_resources* wr, cecs_resource_id id) {
    assert(
        cecs_displaced_set_contains(&wr->resource_handles, (size_t)id, &(cecs_resource_handle){0}, sizeof(cecs_resource_handle))
        && "Resource ID does not correspond to an existing resource"
    );
    return *CECS_DISPLACED_SET_GET(cecs_resource_handle, &wr->resource_handles, (size_t)id);
}

bool cecs_world_resources_remove_resource(cecs_world_resources* wr, cecs_resource_id id) {
    return cecs_displaced_set_remove(&wr->resource_handles, (size_t)id, sizeof(cecs_resource_handle), &(cecs_resource_handle){0});
}

bool cecs_world_resources_remove_resource_out(cecs_world_resources* wr, cecs_resource_id id, cecs_resource_handle out_resource, size_t size) {
    assert(out_resource != NULL && "out_resource must not be NULL, use: cecs_world_resources_remove_resource");
    cecs_resource_handle removed_handle;
    if (cecs_displaced_set_remove_out(&wr->resource_handles, (size_t)id, &removed_handle, size, &(cecs_resource_handle){0})) {
        memcpy(out_resource, removed_handle, size);
        return true;
    }
    else {
        memset(out_resource, 0, size);
        return false;
    }
}
