#ifndef CECS_COMPONENT_H
#define CECS_COMPONENT_H


#ifndef CECS_COMPONENT_TYPE_ID_TYPE
#define CECS_COMPONENT_TYPE_ID_TYPE_DEFAULT uint32_t
#define CECS_COMPONENT_TYPE_ID_TYPE CECS_COMPONENT_TYPE_ID_TYPE_DEFAULT

#define CECS_COMPONENT_TYPE_ID_TYPE_MAX UINT32_MAX
#endif

#ifndef CECS_COMPONENT_TYPE_ID_TYPE_MAX
static_assert(
    false,
    "static error: CECS_COMPONENT_TYPE_ID_TYPE_MAX must be defined when CECS_COMPONENT_TYPE_ID_TYPE is defined"
);
#endif


#include <stdint.h>

typedef enum cecs_component_storage_type {
    cecs_component_storage_type_none = 0,
    cecs_component_storage_type_sparse_set,
} cecs_component_storage_type;
typedef uint8_t cecs_component_storage_value;

typedef CECS_COMPONENT_TYPE_ID_TYPE cecs_component_type_id;
typedef struct cecs_component_type {
    cecs_component_type_id id;
    cecs_component_storage_value storage_type;
} cecs_component_type;

#endif
