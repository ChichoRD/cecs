#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "component.h"

#define COMPONENT_MASK_TYPE uint32_t
typedef COMPONENT_MASK_TYPE component_mask;
typedef uint32_t entity_id;

typedef struct entity
{
    bool active;
    entity_id id;
    component_mask mask;
} entity;

#define ENTITY_HAS_COMPONENT(entity0, component_type) (((entity0).mask & (1 << COMPONENT_ID(component_type))) != 0)
#define ENTITY_HAS_COMPONENTS_ALL(entity0, ...) (((entity0).mask & (COMPONENTS_MASK(__VA_ARGS__))) == COMPONENTS_MASK(__VA_ARGS__))
#define ENTITY_HAS_COMPONENTS_ANY(entity0, ...) (((entity0).mask & (COMPONENTS_MASK(__VA_ARGS__))) != 0)

entity entity_create(bool active, uint32_t id, component_mask mask)
{
    entity e;
    e.active = active;
    e.id = id;
    e.mask = mask;
    return e;
}

#endif