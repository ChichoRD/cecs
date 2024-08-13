#ifndef TAG_H
#define TAG_H

#include <stdint.h>
#include <stdbool.h>
#include "map.h"
#include "type_id.h"
#include "entity.h"

typedef uint32_t tag_id;
static tag_id tag_id_count = 0;

#define TAG(type) type##_tag
#define TAG_IMPLEMENT(type) TYPE_ID_IMPLEMENT_COUNTER(type##_tag, tag_id_count)
#define TAG_DEFINE(type) \
    TAG_IMPLEMENT(type) \
    typedef bool 

#define TAG_ID(type) ((tag_id)TYPE_ID(type##_tag))

#define TAG_MASK(type) (1 << TAG_ID(type))

#define _TAG_MASK_OR(type) TAG_MASK(type) |
#define TAGS_MASK(...) (MAP(_TAG_MASK_OR, __VA_ARGS__) 0)


#define TAG_TYPE_COUNT (8 * sizeof(tag_mask))

#endif