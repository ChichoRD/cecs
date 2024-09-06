#ifndef TAG_H
#define TAG_H

#include <stdint.h>
#include <stdbool.h>
#include "../types/macro_utils.h"
#include "component/entity/entity.h"
#include "component/component_storage.h"

typedef component_id tag_id;

#define TAG(type) CAT2(type, _tag)
#define TAG_IMPLEMENT(type) COMPONENT_IMPLEMENT(type)

#define TAG_ID(type) ((tag_id)COMPONENT_ID(type))
#define TAG_ID_ARRAY(...) (tag_id[]){ MAP(TAG_ID, COMMA, __VA_ARGS__) }

#define TAG_MASK(type) ((tag_mask)(1 << TAG_ID(type)))

#define _TAG_MASK_OR(type) TAG_MASK(type) |
#define TAGS_MASK(...) (FOLD_L1(_TAG_MASK_OR, ((tag_mask)0), MAP(TAG_MASK, COMMA, __VA_ARGS__)))


typedef components_type_info tags_type_info;

#endif