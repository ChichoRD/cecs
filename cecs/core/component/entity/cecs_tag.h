#ifndef CECS_TAG_H
#define CECS_TAG_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../types/cecs_macro_utils.h"
#include "cecs_component_type.h"

typedef cecs_component_id cecs_tag_id;

#define CECS_TAG(type) CAT2(type, _tag)
#define CECS_TAG_IMPLEMENT(type) CECS_COMPONENT_IMPLEMENT(type)

#define CECS_TAG_ID(type) ((cecs_tag_id)CECS_COMPONENT_ID(type))
#define CECS_TAG_ID_ARRAY(...) ((cecs_tag_id[]){ MAP(CECS_TAG_ID, COMMA, __VA_ARGS__) })


#define CECS_TAG_MASK_TYPE uint64_t
typedef CECS_TAG_MASK_TYPE cecs_tag_mask;

#define CECS_TAG_MASK(type) ((cecs_tag_mask)(1 << CECS_TAG_ID(type)))

#define _CECS_TAG_MASK_OR(type) CECS_TAG_MASK(type) |
#define CECS_TAGS_MASK(...) (FOLD_L1(_CECS_TAG_MASK_OR, ((cecs_tag_mask)0), MAP(CECS_TAG_MASK, COMMA, __VA_ARGS__)))


typedef cecs_components_type_info cecs_tags_type_info;

#endif