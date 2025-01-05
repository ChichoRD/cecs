#ifndef CECS_COMPONENT_TYPE_H
#define CECS_COMPONENT_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "../../../types/cecs_macro_utils.h"
#include "../../../types/cecs_type_id.h"

#define CECS_COMPONENT_ID_TYPE uint64_t
typedef CECS_COMPONENT_ID_TYPE cecs_component_id;
extern cecs_component_id *cecs_component_id_counter(void);

#define CECS_COMPONENT_ID_TYPE_BITS_LOG2 6
#define CECS_COMPONENT_ID_TYPE_BITS (1 << CECS_COMPONENT_ID_TYPE_BITS_LOG2)
static_assert(CECS_COMPONENT_ID_TYPE_BITS == (8 * sizeof(cecs_component_id)), "Invalid cecs_component_id type size");

#define CECS_COMPONENT(type) CECS_CAT2(type, _component)

#define _CECS_COMPONENT_DECLARE(component) CECS_TYPE_ID_DECLARE(component)
#define CECS_COMPONENT_DECLARE(type) _CECS_COMPONENT_DECLARE(CECS_COMPONENT(type))

#define _CECS_COMPONENT_DEFINE(component) CECS_TYPE_ID_DEFINE_WITH_COUNTER(component, cecs_component_id_counter())
#define CECS_COMPONENT_DEFINE(type) _CECS_COMPONENT_DEFINE(CECS_COMPONENT(type))

#define CECS_COMPONENT_ID(type) ((cecs_component_id)CECS_TYPE_ID(CECS_COMPONENT(type)))
#define CECS_COMPONENT_ID_ARRAY(...) ((cecs_component_id[]){ CECS_MAP(CECS_COMPONENT_ID, CECS_COMMA, __VA_ARGS__) })
#define CECS_COMPONENT_COUNT(...) (sizeof(CECS_COMPONENT_ID_ARRAY(__VA_ARGS__)) / sizeof(cecs_component_id))


typedef struct cecs_components_type_info {
    cecs_component_id *component_ids;
    size_t component_count;
} cecs_components_type_info;

static inline cecs_components_type_info cecs_components_type_info_create(cecs_component_id *component_ids, size_t component_count) {
    return (cecs_components_type_info) {
        .component_ids = component_ids,
        .component_count = component_count
    };
}

#define CECS_COMPONENTS_TYPE_INFO_CREATE(...) \
    cecs_components_type_info_create( \
        CECS_COMPONENT_ID_ARRAY(__VA_ARGS__), \
        CECS_COMPONENT_COUNT(__VA_ARGS__) \
    )
#define CECS_COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(...) \
    cecs_components_type_info_create( \
        ((cecs_component_id[]){ __VA_ARGS__ }), \
        sizeof((cecs_component_id[]){ __VA_ARGS__ }) / sizeof(cecs_component_id) \
    )

typedef union {
    struct {
        const cecs_component_id *const component_ids;
        const size_t component_count;
    };
    cecs_components_type_info components_type_info;
} cecs_components_type_info_deconstruct;



#endif