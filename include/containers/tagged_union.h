#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <assert.h>
#include <stdint.h>
#include "../types/macro_utils.h"


#define _PREPEND_UNDERSCORE(x) _##x
#define _PREPEND_UNDERSCORE_SELECT(type, identifier) _PREPEND_UNDERSCORE(identifier)
#define TAGGED_UNION(...) CAT(tagged_union, MAP_PAIRS(_PREPEND_UNDERSCORE_SELECT, COMMA, __VA_ARGS__))

#define _PREPEND_VALUE(x) value_##x
#define _TAGGED_UNION_FIELD(type, identifier) type _PREPEND_VALUE(identifier);
#define _PREPEND_VARIANT(x) Variant_##x
#define _PREPEND_VARIANT_SELECT(type, identifier) _PREPEND_VARIANT(identifier)
#define _TAGGED_UNION_STRUCT(identifier, ...) \
    struct identifier { \
        union { \
            MAP_PAIRS(_TAGGED_UNION_FIELD, EMPTY, __VA_ARGS__) \
        }; \
        enum { \
            MAP_PAIRS(_PREPEND_VARIANT_SELECT, COMMA, __VA_ARGS__), \
        } variant; \
    }

#define TAGGED_UNION_STRUCT(...) _TAGGED_UNION_STRUCT(TAGGED_UNION(__VA_ARGS__), __VA_ARGS__)

#define TAGGED_UNION_VALUE(identifier) _PREPEND_VALUE(identifier)
#define TAGGED_UNION_VARIANT(identifier) _PREPEND_VARIANT(identifier)
#define TAGGED_UNION_CREATE(identifier, value) \
    { .TAGGED_UNION_VALUE(identifier) = { (value) }, .variant = TAGGED_UNION_VARIANT(identifier) }
#define TAGGED_UNION_STRUCT_CREATE(identifier, value, ...) \
    (TAGGED_UNION_STRUCT(__VA_ARGS__)) TAGGED_UNION_CREATE(identifier, (value))

#define TAGGED_UNION_IS(identifier, union) ((union).variant == TAGGED_UNION_VARIANT(identifier))
#define TAGGED_UNION_IS_ASSERT(identifier, union) (assert(TAGGED_UNION_IS(identifier, (union)) && "Invalid union access"))
#define TAGGED_UNION_GET(identifier, union) \
    (TAGGED_UNION_IS_ASSERT(identifier, (union)), (union).TAGGED_UNION_VALUE(identifier))

#define MATCH(union) switch ((union).variant)


typedef uint8_t none;
#define NONE 0
#define OPTION(type) option_##type
#define OPTION_STRUCT(type, identifier) _TAGGED_UNION_STRUCT(OPTION(identifier), none, None, type, identifier)

#define OPTION_CREATE(identifier, value) TAGGED_UNION_CREATE(identifier, value)
#define OPTION_CREATE_STRUCT(identifier, value) (OPTION_STRUCT(identifier)) OPTION_CREATE(identifier, value)
#define OPTION_HAS(option) ((option).variant != TAGGED_UNION_VARIANT(None))
#define OPTION_HAS_ASSERT(option) (assert(OPTION_HAS(option) && "Invalid option access"))
#define OPTION_GET(identifier, option) (OPTION_HAS_ASSERT(option), (option).TAGGED_UNION_VALUE(identifier))

#endif