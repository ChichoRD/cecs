#ifndef COMPONENT_TYPE_H
#define COMPONENT_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "../../types/macro_utils.h"
#include "../../types/type_id.h"

#define COMPONENT_ID_TYPE uint64_t
typedef COMPONENT_ID_TYPE component_id;

#define COMPONENT_ID_TYPE_BITS_LOG2 6
#define COMPONENT_ID_TYPE_BITS (1 << COMPONENT_ID_TYPE_BITS_LOG2)
static_assert(COMPONENT_ID_TYPE_BITS == (8 * sizeof(component_id)), "Invalid component_id type size");

static component_id component_id_count = 0;

#define COMPONENT(type) CAT2(type, _component)
#define _COMPONENT_IMPLEMENT(component) TYPE_ID_IMPLEMENT_COUNTER(component, component_id_count)
#define COMPONENT_IMPLEMENT(type) _COMPONENT_IMPLEMENT(COMPONENT(type))

#define COMPONENT_ID(type) ((component_id)TYPE_ID(COMPONENT(type)))
#define COMPONENT_ID_ARRAY(...) ((component_id[]){ MAP(COMPONENT_ID, COMMA, __VA_ARGS__) })
#define COMPONENT_COUNT(...) (sizeof(COMPONENT_ID_ARRAY(__VA_ARGS__)) / sizeof(component_id))


#define COMPONENT_MASK_TYPE uint64_t
typedef COMPONENT_MASK_TYPE component_mask;

#define COMPONENT_MASK(type) ((component_mask)(1 << COMPONENT_ID(type)))

#define _COMPONENT_MASK_OR(mask, next_mask) (mask | next_mask)
#define COMPONENTS_MASK(...) (FOLD_L1(_COMPONENT_MASK_OR, ((component_mask)0), MAP(COMPONENT_MASK, COMMA, __VA_ARGS__)))

typedef struct components_type_info {
    const component_id *const component_ids;
    const size_t component_count;
} components_type_info;

inline components_type_info components_type_info_create(const component_id *component_ids, size_t component_count) {
    return (components_type_info) {
        .component_ids = component_ids,
        .component_count = component_count
    };
}

#define COMPONENTS_TYPE_INFO_CREATE(...) \
    components_type_info_create( \
        COMPONENT_ID_ARRAY(__VA_ARGS__), \
        COMPONENT_COUNT(__VA_ARGS__) \
    )
#define COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(...) \
    components_type_info_create( \
        ((component_id[]){ __VA_ARGS__ }), \
        sizeof((component_id[]){ __VA_ARGS__ }) / sizeof(component_id) \
    )

typedef union {
    struct {
        const component_id *const component_ids;
        const size_t component_count;
    };
    components_type_info components_type_info;
} components_type_info_deconstruct;



#endif