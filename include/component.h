#ifndef COMPONENT_H
#define COMPONENT_H

#include <stdint.h>
#include "type_id.h"

typedef uint32_t component_id;
static component_id component_id_count = 0;

#define COMPONENT_DEFINE(type) TYPE_ID_DEFINE_COUNTER(type##_component, component_id_count)
#define COMPONENT_ID(type) (component_id)TYPE_ID(type##_component)

#endif