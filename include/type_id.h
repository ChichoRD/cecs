#ifndef TYPE_ID_H
#define TYPE_ID_H

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t type_id;
static type_id type_id_count = 0;

#define TYPE_ID_DEFINE_COUNTER(type, counter) \
    static bool type##_initialized_type_id = false; \
    static type_id type##_id = 0; \
    type_id type##_type_id() { \
        if (!type##_initialized_type_id) { \
            type##_initialized_type_id = true; \
            type##_id = counter++; \
        } \
        return type##_id; \
    }; \
    typedef struct type \

#define TYPE_ID_DEFINE(type) TYPE_ID_DEFINE_COUNTER(type, type_id_count)

#define TYPE_ID(type) type##_type_id()

TYPE_ID_DEFINE(test) {
    int a;
} test;

#endif