#ifndef RELATION_H
#define RELATION_H

#include "component.h"
#include "entity.h"
#include "tag.h"

#define RELATION(tag, type) tag##_##type##_relation
#define RELATION_IMPLEMENT(tag_type, type) \
    struct RELATION(tag_type, type) tag_type##_##type##_relation##_create(tag_type tag, type component) { \
        return (struct RELATION(tag_type, type)) { \
            .tag = tag, \
            .component = component \
        }; \
    }

#define RELATION_DEFINE(tag_type, type) \
    COMPONENT_IMPLEMENT(tag_type##_##type##_relation) \
    typedef struct RELATION(tag_type, type) { \
        tag_type tag; \
        type component; \
    } RELATION(tag_type, type); \
    RELATION_IMPLEMENT(tag_type, type)

#define RELATION_ID(tag, type) TAG_ID(tag)
inline type_id TYPE_ID(TAG(entity_id)) {
    return TAG_TYPE_COUNT;
}

#define _TAG_OR_ENTITY(tag_type, entity_id_value) \
    (TAG_ID(tag_type) == TAG_ID(entity_id) ? entity_id_value : TAG_ID(tag_type))
#define RELATION_CREATE(tag_type, tag, component_type, component) \
    tag_type##_##component_type##_relation##_create(_TAG_OR_ENTITY(tag_type, tag), component)

#endif