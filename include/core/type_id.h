#ifndef TYPE_ID_H
#define TYPE_ID_H

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t type_id;
static type_id type_id_count = 0;

#define TYPE_ID_IMPLEMENT_COUNTER(type, counter) \
    static bool type##_initialized_type_id = false; \
    static type_id type##_id = 0; \
    type_id type##_type_id() { \
        if (!type##_initialized_type_id) { \
            type##_initialized_type_id = true; \
            type##_id = counter++; \
        } \
        return type##_id; \
    }

#define TYPE_ID_IMPLEMENT(type) TYPE_ID_IMPLEMENT_COUNTER(type, type_id_count)
#define TYPE_ID(type) type##_type_id()

#define TYPE_ID_DEFINE(layout, type) \
    TYPE_ID_IMPLEMENT(type) \
    typedef layout type

#endif