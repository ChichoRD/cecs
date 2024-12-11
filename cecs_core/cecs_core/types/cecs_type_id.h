#ifndef CECS_TYPE_ID_H
#define CECS_TYPE_ID_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t cecs_type_id;
extern cecs_type_id cecs_type_id_count;

#define CECS_PASTE(x, y) x##y
#define CECS_PASTE3(x, y, z) x##y##z

#define CECS_TYPE_ID_IMPLEMENT_COUNTER(type, counter) \
    static bool cecs_##type##_initialized_type_id = false; \
    static cecs_type_id cecs_##type##_id = 0; \
    static cecs_type_id CECS_TYPE_ID_VOID(type); \
    cecs_type_id CECS_TYPE_ID_VOID(type) { \
        if (!cecs_##type##_initialized_type_id) { \
            cecs_##type##_initialized_type_id = true; \
            cecs_##type##_id = counter++; \
        } \
        return cecs_##type##_id; \
    }

#define CECS_TYPE_ID_IMPLEMENT(type) CECS_TYPE_ID_IMPLEMENT_COUNTER(type, cecs_type_id_count)
#define CECS_TYPE_ID_VOID(type) CECS_PASTE3(cecs_, type, _type_id(void))
#define CECS_TYPE_ID(type) CECS_PASTE3(cecs_, type, _type_id())

#endif