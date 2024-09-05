#ifndef TAG_H
#define TAG_H

#include <stdint.h>
#include <stdbool.h>
#include "../types/macro_utils.h"
#include "../types/type_id.h"
#include "component/entity/entity.h"

typedef uint64_t tag_id;
static tag_id tag_id_count = 0;

#define TAG(type) CAT2(type, _tag)
#define _TAG_IMPLEMENT(tag) TYPE_ID_IMPLEMENT_COUNTER(tag, tag_id_count)
#define TAG_IMPLEMENT(type) _TAG_IMPLEMENT(TAG(type))

#define TAG_ID(type) ((tag_id)TYPE_ID(TAG(type)))
#define TAG_ID_ARRAY(...) (tag_id[]){ MAP(TAG_ID, COMMA, __VA_ARGS__) }

#define TAG_MASK(type) ((tag_mask)(1 << TAG_ID(type)))

#define _TAG_MASK_OR(type) TAG_MASK(type) |
#define TAGS_MASK(...) (FOLD_L1(_TAG_MASK_OR, ((tag_mask)0), MAP(TAG_MASK, COMMA, __VA_ARGS__)))


#define TAG_TYPE_COUNT (8 * sizeof(tag_mask))

#endif