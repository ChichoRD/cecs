#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>

#define COMPONENT_MASK uint32_t

typedef uint32_t entity_id;

typedef struct entity
{
    bool active;
    entity_id id;
    COMPONENT_MASK mask;
} entity;

entity entity_create(bool active, uint32_t id, COMPONENT_MASK mask)
{
    entity e;
    e.active = active;
    e.id = id;
    e.mask = mask;
    return e;
}

#endif