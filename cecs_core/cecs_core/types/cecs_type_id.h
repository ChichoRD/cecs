#ifndef CECS_TYPE_ID_H
#define CECS_TYPE_ID_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef uint64_t cecs_type_id;
extern cecs_type_id *cecs_type_id_counter(void);

#define CECS_PASTE(x, y) x##y
#define CECS_PASTE3(x, y, z) x##y##z

typedef struct cecs_type_id_meta {
    bool initialized;
    cecs_type_id id;
} cecs_type_id_meta;

#define CECS_TYPE_ID_META(type) CECS_PASTE3(cecs_, type, _type_id_meta)
#define CECS_TYPE_ID_FUNC_NAME(type) CECS_PASTE3(cecs_, type, _type_id)
#define CECS_TYPE_ID_FUNC(type) CECS_TYPE_ID_FUNC_NAME(type)(void)

#define CECS_TYPE_ID_DECLARE(type) \
    extern cecs_type_id CECS_TYPE_ID_FUNC(type)

#define CECS_TYPE_ID_DEFINE_WITH_COUNTER(type, counter_ref) \
    static cecs_type_id_meta CECS_TYPE_ID_META(type) = { .initialized = false, .id = 0 }; \
    cecs_type_id CECS_TYPE_ID_FUNC(type) { \
        if (!CECS_TYPE_ID_META(type).initialized) { \
            CECS_TYPE_ID_META(type).initialized = true; \
            CECS_TYPE_ID_META(type).id = (*counter_ref)++; \
        } \
        return CECS_TYPE_ID_META(type).id; \
    }

#define CECS_TYPE_ID_DEFINE(type) CECS_TYPE_ID_DEFINE_WITH_COUNTER(type, cecs_type_id_counter())

#define CECS_TYPE_ID(type) CECS_TYPE_ID_FUNC_NAME(type)()

#endif