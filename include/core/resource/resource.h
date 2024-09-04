#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdint.h>
#include "map.h"
#include "type_id.h"

typedef uint32_t resource_id;
static resource_id resource_id_count = 0;

#define RESOURCE(type) type##_resource
#define RESOURCE_IMPLEMENT(type) TYPE_ID_IMPLEMENT_COUNTER(type##_resource, resource_id_count)
#define RESOURCE_DEFINE(layout, type) \
    RESOURCE_IMPLEMENT(type) \
    typedef layout type

#define RESOURCE_ID(type) ((resource_id)TYPE_ID(type##_resource))
#define RESOURCE_ID_ARRAY(...) (resource_id[]){ MAP_LIST(RESOURCE_ID, __VA_ARGS__) }


#define DEFAULT_RESOURCE_CAPACITY 64
#define DEFAULT_RESOURCE_SIZE 64

typedef ssize_t resource_offset;
typedef struct world_resources {
    list resources;
    list resource_offsets;
} world_resources;

world_resources world_resources_create(arena *resources_arena) {
    world_resources wr;
    wr.resources = list_create_with_capacity(resources_arena, (DEFAULT_RESOURCE_SIZE * DEFAULT_RESOURCE_CAPACITY));
    wr.resource_offsets = list_create_with_capacity(resources_arena, sizeof(resource_offset) * DEFAULT_RESOURCE_CAPACITY);
    return wr;
}

inline size_t world_resources_count(const world_resources *wr) {
    return list_count_of_size(&wr->resource_offsets, sizeof(resource_offset));
}

void *world_resources_add_resource(world_resources *wr, arena *resources_arena, resource_id id, void *resource, size_t size) {
    resource_offset offset = (id < world_resources_count(wr))
        ? *LIST_GET(resource_offset, &wr->resource_offsets, id)
        : wr->resources.count;
    if (offset >= 0) {
        assert((id == world_resources_count(wr)) && "Resource ID out of bounds");
        LIST_ADD(resource_offset, &wr->resource_offsets, resources_arena, &offset);
        return list_add_range(&wr->resources, resources_arena, resource, size, sizeof(uint8_t));
    } else {
        if (id >= world_resources_count(wr)) {
            assert(0 && "unreachable: resource_offsets and resources are out of sync");
            exit(EXIT_FAILURE);
        }
        resource_offset removed_offset = -offset - 1;
        LIST_SET(resource_offset, &wr->resource_offsets, id, &removed_offset);
        return (uint8_t *)wr->resources.elements + removed_offset;
    }
}

void *world_resources_get_resource(const world_resources *wr, resource_id id, size_t size) {
    assert((id < world_resources_count(wr)) && "Resource ID out of bounds");
    resource_offset offset = *LIST_GET(resource_offset, &wr->resource_offsets, id);
    assert((offset >= 0) && "Resource has been removed");

    void *resource = (uint8_t *)wr->resources.elements + offset;
    assert(((uint8_t *)resource + size <= (uint8_t *)wr->resources.elements + wr->resources.count) && "Resource out of bounds");
    return resource;
}

void *world_resources_set_resource(world_resources *wr, resource_id id, void *resource, size_t size) {
    assert((id < world_resources_count(wr)) && "Resource ID out of bounds");
    resource_offset offset = *LIST_GET(resource_offset, &wr->resource_offsets, id);
    assert((offset >= 0) && "Resource has been removed");

    void *old_resource = (uint8_t *)wr->resources.elements + offset;
    assert(((uint8_t *)old_resource + size <= (uint8_t *)wr->resources.elements + wr->resources.count) && "Resource out of bounds");
    return list_set_range(&wr->resources, offset, resource, size, sizeof(uint8_t));
}

void *world_resources_remove_resource(world_resources *wr, resource_id id, size_t size) {
    void *resource = world_resources_get_resource(wr, id, size);

    resource_offset removed_offset = (uint8_t *)wr->resources.elements - (uint8_t *)resource - 1;
    LIST_SET(resource_offset, &wr->resource_offsets, id, &removed_offset);
    return resource;
}

#endif