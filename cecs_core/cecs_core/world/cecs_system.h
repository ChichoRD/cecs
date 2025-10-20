#ifndef CECS_SYSTEM_H
#define CECS_SYSTEM_H

#include <stdint.h>

#ifndef CECS_SYSTEM_ID_VALUE_TYPE
#define CECS_SYSTEM_ID_VALUE_TYPE_DEFAULT uint32_t
#define CECS_SYSTEM_ID_VALUE_TYPE CECS_SYSTEM_ID_VALUE_TYPE_DEFAULT

#endif


typedef CECS_SYSTEM_ID_VALUE_TYPE cecs_system_id_value;
typedef struct cecs_system_id {
    cecs_system_id_value value;
} cecs_system_id;

inline cecs_system_id cecs_system_id_from_value(const cecs_system_id_value value) {
    return (cecs_system_id){ .value = value };
}

#endif
