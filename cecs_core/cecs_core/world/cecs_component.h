#ifndef CECS_COMPONENT_H
#define CECS_COMPONENT_H

// TODO!

#include <stdint.h>

typedef enum cecs_component_storage_type {
    cecs_component_storage_type_none = 0,
    cecs_component_storage_type_sparse_set,
} cecs_component_storage_type;
typedef uint8_t cecs_component_storage_value;

typedef struct cecs_component_type {
    uint32_t id;
    cecs_component_storage_value storage_type;
} cecs_component_type;

#endif
