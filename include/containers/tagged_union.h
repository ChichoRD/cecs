#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <assert.h>
#include <stdint.h>
#include "../core/map.h"


#define _PREPEND_UNDERSCORE(x) _##x
#define TAGGED_UNION(...) CAT(tagged_union, MAP_LIST(_PREPEND_UNDERSCORE, __VA_ARGS__))

#define _PREPEND_VALUE(x) value_##x
#define _TAGGED_UNION_FIELD(type) type _PREPEND_VALUE(type);
#define _PREPEND_VARIANT(x) Variant_##x
#define _TAGGED_UNION_STRUCT(identifier, ...) \
    struct identifier { \
        union { \
            MAP(_TAGGED_UNION_FIELD, __VA_ARGS__) \
        }; \
        enum { \
            MAP_LIST(_PREPEND_VARIANT, __VA_ARGS__), \
        } variant; \
    }

#define TAGGED_UNION_STRUCT(...) _TAGGED_UNION_STRUCT(TAGGED_UNION(__VA_ARGS__), __VA_ARGS__)

#define TAGGED_UNION_VALUE(type) _PREPEND_VALUE(type)
#define TAGGED_UNION_VARIANT(type) _PREPEND_VARIANT(type)
#define TAGGED_UNION_CREATE(type, value) \
    { .TAGGED_UNION_VALUE(type) = { (value) }, .variant = TAGGED_UNION_VARIANT(type) }
#define TAGGED_UNION_STRUCT_CREATE(type, value, ...) \
    (TAGGED_UNION_STRUCT(__VA_ARGS__)) TAGGED_UNION_CREATE(type, (value))

#define TAGGED_UNION_IS(type, union) ((union).variant == TAGGED_UNION_VARIANT(type))
#define TAGGED_UNION_IS_ASSERT(type, union) (assert(TAGGED_UNION_IS(type, (union)) && "Invalid union access"))
#define TAGGED_UNION_GET(type, union) \
    (TAGGED_UNION_IS_ASSERT(type, (union)), (union).TAGGED_UNION_VALUE(type))

#define MATCH(union) switch ((union).variant)


typedef uint8_t none;
#define NONE 0
#define OPTION(type) option_##type
#define OPTION_STRUCT(type) _TAGGED_UNION_STRUCT(OPTION(type), none, type)

#define OPTION_CREATE(type, value) TAGGED_UNION_CREATE(type, value)
#define OPTION_CREATE_STRUCT(type, value) (OPTION_STRUCT(type)) OPTION_CREATE(type, value)
#define OPTION_HAS(option) ((option).variant != TAGGED_UNION_VARIANT(none))
#define OPTION_HAS_ASSERT(option) (assert(OPTION_HAS(option) && "Invalid option access"))
#define OPTION_GET(type, option) (OPTION_HAS_ASSERT(option), (option).TAGGED_UNION_VALUE(type))

#endif