#ifndef CECS_TAG_H
#define CECS_TAG_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../types/cecs_macro_utils.h"
#include "cecs_component_type.h"

typedef cecs_component_id cecs_tag_id;
static inline cecs_tag_id *cecs_tag_id_counter(void) {
    return cecs_component_id_counter();
}

#define CECS_TAG_DECLARE(type) CECS_COMPONENT_DECLARE(type)
#define CECS_TAG_DEFINE(type) CECS_COMPONENT_DEFINE(type)

#define CECS_TAG_ID(type) ((cecs_tag_id)CECS_COMPONENT_ID(type))
#define CECS_TAG_ID_ARRAY(...) ((cecs_tag_id[]){ MAP(CECS_TAG_ID, COMMA, __VA_ARGS__) })

typedef cecs_components_type_info cecs_tags_type_info;

#endif