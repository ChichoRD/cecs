#ifndef COMPONENT_H
#define COMPONENT_H

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include "map.h"
#include "type_id.h"
#include "entity.h"

typedef uint32_t component_id;
static component_id component_id_count = 0;

#define COMPONENT(type) type##_component
#define COMPONENT_IMPLEMENT(type) TYPE_ID_IMPLEMENT_COUNTER(type##_component, component_id_count)
#define COMPONENT_DEFINE(layout, type) \
    COMPONENT_IMPLEMENT(type) \
    typedef layout type##_component

#define COMPONENT_ID(type) ((component_id)TYPE_ID(type##_component))
#define COMPONENT_ID_ARRAY(...) (component_id[]){ MAP_LIST(COMPONENT_ID, __VA_ARGS__) }

#define COMPONENT_MASK(type) (1 << COMPONENT_ID(type))

#define _COMPONENT_MASK_OR(type) COMPONENT_MASK(type) |
#define COMPONENTS_MASK(...) (MAP(_COMPONENT_MASK_OR, __VA_ARGS__) 0)


#define COMPONENT_TYPE_COUNT (8 * sizeof(component_mask))
#define DEFAULT_COMPONENT_SIZE 64

typedef int32_t ssize_t;

typedef struct world_components {
    list components[COMPONENT_TYPE_COUNT];
    ssize_t component_sizes[COMPONENT_TYPE_COUNT];
} world_components;

world_components world_components_create(arena *components_arena, size_t entity_count) {
    world_components wc;
    for (size_t i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        wc.components[i] = list_create(components_arena, DEFAULT_COMPONENT_SIZE * entity_count);
        wc.component_sizes[i] = -1;
    }
    return wc;
}

void *world_components_add_component(world_components *wc, arena *components_arena, size_t entity_count, component_id component_id, void *component, size_t size) {
    assert((component_id < COMPONENT_TYPE_COUNT) && "Component ID out of bounds");
    if (wc->component_sizes[component_id] == -1) {
        if (wc->components[component_id].count != 0) {
            assert(0 && "unreachable: component_sizes and components are out of sync");
            exit(EXIT_FAILURE);
        }
        wc->component_sizes[component_id] = size;
        
        void *last_added = NULL;
        for (size_t i = 0; i < entity_count; i++) {
            last_added = list_add(&wc->components[component_id], components_arena, component, size);
        }
        return last_added;
    } else {
        assert((wc->component_sizes[component_id] == (ssize_t)size) && "Component size mismatch");
        return list_add(&wc->components[component_id], components_arena, component, size);
    }
}

void *world_components_set_component(world_components *wc, component_id component_id, entity_id entity_id, void *component, size_t size) {
    assert((component_id < COMPONENT_TYPE_COUNT) && "Component ID out of bounds");
    assert((entity_id < list_count_of_size(&wc->components[component_id], size)) && "Entity ID out of bounds");
    assert((wc->component_sizes[component_id] == (ssize_t)size) && "Component size mismatch");
    return list_set(&wc->components[component_id], entity_id, component, size);
}

void *world_components_get_component(const world_components *wc, component_id component_id, entity_id entity_id, size_t size) {
    assert((component_id < COMPONENT_TYPE_COUNT) && "Component ID out of bounds");
    assert((entity_id < list_count_of_size(&wc->components[component_id], size)) && "Entity ID out of bounds");
    assert((wc->component_sizes[component_id] == (ssize_t)size) && "Component size mismatch");
    return list_get(&wc->components[component_id], entity_id, size);
}

void world_components_grow_all(world_components *wc, arena *components_arena, size_t entity_count) {
    for (size_t i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        ssize_t size = wc->component_sizes[i];
        if (size != -1) {
            if (list_count_of_size(&wc->components[i], size) <= 0) {
                assert(0 && "unreachable: component_sizes and components are out of sync");
                exit(EXIT_FAILURE);
            }     
            world_components_add_component(wc, components_arena, entity_count, i, list_get(&wc->components[i], 0, size), size);
        }
    }
}

#endif