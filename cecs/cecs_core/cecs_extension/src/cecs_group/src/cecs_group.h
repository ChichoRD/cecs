#ifndef CECS_GROUP_H
#define CECS_GROUP_H

#ifndef CECS_GROUP_TYPE_ID_TYPE
#define CECS_GROUP_TYPE_ID_TYPE_DEFAULT uint32_t
#define CECS_GROUP_TYPE_ID_TYPE CECS_GROUP_TYPE_ID_TYPE_DEFAULT
#define CECS_GROUP_TYPE_ID_TYPE_MAX UINT32_MAX
#endif

#ifndef CECS_GROUP_TYPE_ID_TYPE_MAX
static_assert(
    false,
    "static error: CECS_GROUP_TYPE_ID_TYPE_MAX must be defined when CECS_GROUP_TYPE_ID_TYPE is defined"
);
#endif


#include "group/cecs_sparse_set_group.h"

typedef enum cecs_group_storage_type {
    cecs_group_storage_type_none = 0,
    cecs_group_storage_type_sparse_set,
} cecs_group_storage_type;
typedef uint8_t cecs_group_storage_value;

typedef CECS_GROUP_TYPE_ID_TYPE cecs_group_type_id;
typedef struct cecs_group_type {
    cecs_group_type_id id;
    cecs_group_storage_value storage_type;
} cecs_group_type;

typedef union cecs_group {
    cecs_sparse_set_group sparse_set;
} cecs_group;

#endif
