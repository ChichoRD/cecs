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
    wr.resource_handles = cecs_sentinel_set_create_with_capacity(&wr.resources_arena, sizeof(cecs_resource_handle) * resource_capactity);
    wr.discard = cecs_discard_create();
    return wr;
}

void cecs_world_resources_free(cecs_world_resources* wr) {
    cecs_arena_free(&wr->resources_arena);
    wr->resource_handles = (cecs_sentinel_set){ 0 };
    wr->discard = (cecs_resource_discard){ 0 };
}

bool cecs_world_resources_has_resource(const cecs_world_resources* wr, cecs_resource_id id) {
    return cecs_sentinel_set_contains_index(&wr->resource_handles, (size_t)id)
        && (*(const cecs_resource_handle *)cecs_sentinel_set_get_inbounds_const(
            &wr->resource_handles, (size_t)id, sizeof(cecs_resource_handle)
        ) != NULL
    );
}

cecs_resource_handle cecs_world_resources_set_resource(cecs_world_resources* wr, cecs_resource_id id, void* resource, size_t size) {
    cecs_discard_ensure(&wr->discard, &wr->resources_arena, size);

    cecs_resource_handle handle = cecs_world_resources_has_resource(wr, id)
        ? *CECS_SENTINEL_SET_GET_INBOUNDS(cecs_resource_handle, &wr->resource_handles, (size_t)id)
        : cecs_arena_alloc(&wr->resources_arena, size);
    memcpy(handle, resource, size);

    cecs_sentinel_set_expand_to_include(
        &wr->resource_handles, &wr->resources_arena, cecs_inclusive_range_singleton(id), sizeof(cecs_resource_handle), 0
    );
    return *CECS_SENTINEL_SET_SET_INBOUNDS(
        cecs_resource_handle,
        &wr->resource_handles,
        (size_t)id,
        &handle
    );
}

cecs_resource_handle cecs_world_resources_get_resource(const cecs_world_resources* wr, cecs_resource_id id) {
    cecs_resource_handle handle = *CECS_SENTINEL_SET_GET_INBOUNDS(cecs_resource_handle, &wr->resource_handles, (size_t)id);
    assert(handle != NULL && "error: resource does not exist, handle is NULL");
    return handle;
}

bool cecs_world_resources_remove_resource(cecs_world_resources* wr, cecs_resource_id id) {
    cecs_resource_handle handle = NULL;
    bool removed = cecs_sentinel_set_remove(
        &wr->resource_handles, &wr->resources_arena, (size_t)id, &handle, sizeof(cecs_resource_handle), 0
    );
    return removed && handle != NULL;
}

bool cecs_world_resources_remove_resource_out(cecs_world_resources* wr, cecs_resource_id id, cecs_resource_handle out_resource, size_t size) {
    assert(out_resource != NULL && "out_resource must not be NULL, use: cecs_world_resources_remove_resource");
    cecs_resource_handle handle = NULL;
    bool removed = cecs_sentinel_set_remove(
        &wr->resource_handles, &wr->resources_arena, (size_t)id, &handle, sizeof(cecs_resource_handle), 0
    );

    if (removed && handle != NULL) {
        memcpy(out_resource, handle, size);
        return true;
    } else {
        memset(out_resource, 0, size);
        return false;
    }
}
