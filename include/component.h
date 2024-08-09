#ifndef COMPONENT_H
#define COMPONENT_H

#include <stdint.h>
#include "map.h"
#include "type_id.h"

typedef uint32_t component_id;
static component_id component_id_count = 0;

#define COMPONENT_DEFINE(type) TYPE_ID_DEFINE_COUNTER(type##_component, component_id_count)
#define COMPONENT_ID(type) (component_id)TYPE_ID(type##_component)
#define COMPONENT_ID_ARRAY(...) (component_id[]){ MAP_LIST(COMPONENT_ID, __VA_ARGS__) }

#define COMPONENT_MASK(type) (1 << COMPONENT_ID(type))

#define _COMPONENT_MASK_OR(type) COMPONENT_MASK(type) |
#define COMPONENTS_MASK(...) (MAP(_COMPONENT_MASK_OR, __VA_ARGS__) 0)

#endif