#ifndef CECS_COMPONENT_TYPE_H
#define CECS_COMPONENT_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "../../../types/cecs_macro_utils.h"
#include "../../../types/cecs_type_id.h"

#define CECS_COMPONENT_ID_TYPE uint64_t
typedef CECS_COMPONENT_ID_TYPE cecs_component_id;

#define CECS_COMPONENT_ID_TYPE_BITS_LOG2 6
#define CECS_COMPONENT_ID_TYPE_BITS (1 << CECS_COMPONENT_ID_TYPE_BITS_LOG2)
static_assert(CECS_COMPONENT_ID_TYPE_BITS == (CHAR_BIT * sizeof(cecs_component_id)), "Invalid cecs_component_id type size");

extern cecs_component_id *cecs_component_id_counter(void);

typedef enum cecs_component_config_storage {
    cecs_component_config_storage_sparse_array,
    cecs_component_config_storage_dense_set,
    cecs_component_config_storage_flatmap
} cecs_component_config_storage;
typedef uint8_t cecs_component_config_storage_type;

typedef struct cecs_component_config {
    cecs_component_config_storage_type storage_type;
} cecs_component_config;

#define CECS_COMPONENT_CONFIG_FUNC_NAME(type) CECS_PASTE3(cecs_, type, _component_config)
#define CECS_COMPONENT_CONFIG_FUNC(type) CECS_COMPONENT_CONFIG_FUNC_NAME(type)(void)
#define CECS_COMPONENT_CONFIG_DEFAULT { .storage_type = cecs_component_config_storage_sparse_array }

typedef struct cecs_component_id_meta {
    cecs_component_config configuration;
} cecs_component_id_meta;

#define CECS_COMPONENT_ID_META(type) CECS_PASTE3(cecs_, type, _component_id_meta)


#define CECS_COMPONENT(type) CECS_CAT2(type, _component)
#define _CECS_COMPONENT_DECLARE(component) \
    CECS_TYPE_ID_DECLARE(component); \
    extern cecs_component_config CECS_COMPONENT_CONFIG_FUNC(component)
#define CECS_COMPONENT_DECLARE(type) _CECS_COMPONENT_DECLARE(CECS_COMPONENT(type))

#define _CECS_COMPONENT_DEFINE_CONFIG(component, config) \
    CECS_TYPE_ID_DEFINE_WITH_COUNTER(component, cecs_component_id_counter()); \
    static const cecs_component_id_meta CECS_COMPONENT_ID_META(component) = { .configuration = config }; \
    cecs_component_config CECS_COMPONENT_CONFIG_FUNC(component) { \
        return CECS_COMPONENT_ID_META(component).configuration; \
    }
#define CECS_COMPONENT_DEFINE_CONFIG(type, config) _CECS_COMPONENT_DEFINE_CONFIG(CECS_COMPONENT(type), config)
#define CECS_COMPONENT_DEFINE(type) CECS_COMPONENT_DEFINE_CONFIG(type, CECS_COMPONENT_CONFIG_DEFAULT)


#define CECS_COMPONENT_ID(type) ((cecs_component_id)CECS_TYPE_ID(CECS_COMPONENT(type)))
#define CECS_COMPONENT_ID_ARRAY(...) ((cecs_component_id[]){ CECS_MAP(CECS_COMPONENT_ID, CECS_COMMA, __VA_ARGS__) })
#define CECS_COMPONENT_COUNT(...) (sizeof(CECS_COMPONENT_ID_ARRAY(__VA_ARGS__)) / sizeof(cecs_component_id))

#define _CECS_COMPONENT_CONFIG(component) CECS_COMPONENT_CONFIG_FUNC_NAME(component)()
#define  CECS_COMPONENT_CONFIG(type) (_CECS_COMPONENT_CONFIG(CECS_COMPONENT(type)))

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