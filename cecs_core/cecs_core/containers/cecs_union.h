#ifndef CECS_UNION_H
#define CECS_UNION_H

#include <assert.h>
#include <stdint.h>
#include "../types/cecs_macro_utils.h"


#define _CECS_PREPEND_UNDERSCORE(x) _##x
#define _CECS_PREPEND_UNDERSCORE_SELECT(type, identifier) _CECS_PREPEND_UNDERSCORE(identifier)
#define CECS_UNION(...) CECS_CAT(cecs_union, CECS_MAP_PAIRS(_CECS_PREPEND_UNDERSCORE_SELECT, CECS_COMMA, __VA_ARGS__))

#define _CECS_PREPEND_VALUE(x) value_##x
#define _CECS_UNION_FIELD(type, identifier) type _CECS_PREPEND_VALUE(identifier);
#define _CECS_PREPEND_VARIANT(x) Variant_##x
#define _CECS_PREPEND_VARIANT_SELECT(type, identifier) _CECS_PREPEND_VARIANT(identifier)
#define _CECS_UNION_VARIANT_FIELD(prefix, identifier) CECS_CAT3(prefix, _, identifier)
#define _CECS_UNION_STRUCT(identifier, prefix, ...) \
    struct identifier { \
        union { \
            CECS_MAP_PAIRS(_CECS_UNION_FIELD, CECS_EMPTY, __VA_ARGS__) \
        }; \
        enum { \
            CECS_MAP_CONST1(_CECS_UNION_VARIANT_FIELD, prefix, CECS_COMMA, CECS_MAP_PAIRS(_CECS_PREPEND_VARIANT_SELECT, CECS_COMMA, __VA_ARGS__)), \
        } variant; \
    }

#define CECS_UNION_STRUCT(prefix, ...) _CECS_UNION_STRUCT(CECS_UNION(__VA_ARGS__), prefix, __VA_ARGS__)

#define CECS_UNION_VALUE(identifier) _CECS_PREPEND_VALUE(identifier)
#define CECS_UNION_VARIANT(identifier, prefix) _CECS_UNION_VARIANT_FIELD(prefix, _CECS_PREPEND_VARIANT(identifier))
#define CECS_UNION_CREATE(identifier, prefix, value) \
    { .CECS_UNION_VALUE(identifier) = (value), .variant = CECS_UNION_VARIANT(identifier, prefix) }
#define CECS_UNION_STRUCT_CREATE(identifier, prefix, value, ...) \
    (CECS_UNION_STRUCT(__VA_ARGS__)) CECS_UNION_CREATE(identifier, prefix, (value))

#define CECS_UNION_CREATE_VARIANT(identifier, variant_value, value) \
    { .CECS_UNION_VALUE(identifier) = (value), .variant = (variant_value) }
#define CECS_UNION_STRUCT_CREATE_VARIANT(identifier, variant_value, value, ...) \
    (CECS_UNION_STRUCT(__VA_ARGS__)) CECS_UNION_CREATE_VARIANT(identifier, (variant_value), (value))

#define CECS_UNION_IS(identifier, prefix, union) ((union).variant == CECS_UNION_VARIANT(identifier, prefix))
#define CECS_UNION_IS_ASSERT(identifier, prefix, union) (assert(CECS_UNION_IS(identifier, prefix, (union)) && "Invalid union access"))
#define CECS_UNION_GET_UNCHECKED(identifier, union) ((union).CECS_UNION_VALUE(identifier))
#define CECS_UNION_GET(identifier, prefix, union) \
    (CECS_UNION_IS_ASSERT(identifier, prefix, (union)), CECS_UNION_GET_UNCHECKED(identifier, (union)))

#define CECS_UNION_MATCH(union) switch ((union).variant)


typedef uint8_t cecs_none;
#define CECS_NONE ((cecs_none)0)
#define CECS_OPTION(identifier) cecs_option_##identifier
#define CECS_OPTION_STRUCT(type, identifier) _CECS_UNION_STRUCT(CECS_OPTION(identifier), identifier, cecs_none, cecs_none, type, identifier)

#define CECS_OPTION_CREATE_SOME(identifier, value) CECS_UNION_CREATE(identifier, identifier, value)
#define CECS_OPTION_CREATE_NONE(identifier) CECS_UNION_CREATE(cecs_none, identifier, CECS_NONE)

#define CECS_OPTION_CREATE_SOME_STRUCT(identifier, value) ((struct CECS_OPTION(identifier))CECS_OPTION_CREATE_SOME(identifier, value))
#define CECS_OPTION_CREATE_NONE_STRUCT(identifier) ((struct CECS_OPTION(identifier))CECS_OPTION_CREATE_NONE(identifier))

#define CECS_OPTION_IS_NONE(identifier, option) ((option).variant == CECS_UNION_VARIANT(cecs_none, identifier))
#define CECS_OPTION_IS_SOME(identifier, option) ((option).variant != CECS_UNION_VARIANT(cecs_none, identifier))
#define CECS_OPTION_IS_NONE_ASSERT(identifier, option) (assert(CECS_OPTION_IS_NONE(identifier, option) && "Invalid option access"))
#define CECS_OPTION_IS_SOME_ASSERT(identifier, option) (assert(CECS_OPTION_IS_SOME(identifier, option) && "Invalid option access"))
#define CECS_OPTION_GET_UNCHECKED(identifier, option) ((option).CECS_UNION_VALUE(identifier))
#define CECS_OPTION_GET(identifier, option) (CECS_OPTION_IS_SOME_ASSERT(identifier, option), CECS_OPTION_GET_UNCHECKED(identifier, option))

#define CECS_OPTION_MAP(identifier, option, some_predicate, none_predicate) \
    (CECS_OPTION_IS_SOME(identifier, option) ? (some_predicate) : (none_predicate))
#define CECS_OPTION_GET_OR_NULL(identifier, option) \
    CECS_OPTION_MAP(identifier, option, CECS_OPTION_GET_UNCHECKED(identifier, option), NULL)
    
#define CECS_OPTION_MAP_REFERENCE(identifier, option, other_identifier) \
    CECS_OPTION_MAP( \
        identifier, \
        option, \
        CECS_OPTION_CREATE_SOME(other_identifier, CECS_OPTION_GET_UNCHECKED(identifier, option)), \
        CECS_OPTION_CREATE_NONE(other_identifier) \
    )
#define CECS_OPTION_MAP_REFERENCE_STRUCT(identifier, option, other_identifier) \
    CECS_OPTION_MAP( \
        identifier, \
        option, \
        CECS_OPTION_CREATE_SOME_STRUCT(other_identifier, CECS_OPTION_GET_UNCHECKED(identifier, option)), \
        CECS_OPTION_CREATE_NONE_STRUCT(other_identifier) \
    )

#define CECS_BORROWED(identifier) borrowed_##identifier
#define CECS_OWNED(identifier) owned_##identifier
#define CECS_COW(identifier) cow_##identifier
#define CECS_COW_STRUCT(type, identifier) \
    _CECS_UNION_STRUCT(CECS_COW(identifier), identifier, const type *, CECS_BORROWED(identifier), type, CECS_OWNED(identifier))

#define CECS_COW_CREATE_OWNED(identifier, value) CECS_UNION_CREATE(CECS_OWNED(identifier), identifier, value)
#define CECS_COW_CREATE_OWNED_STRUCT(identifier, value) ((struct CECS_COW(identifier))CECS_COW_CREATE_OWNED(identifier, value))

#define CECS_COW_CREATE_BORROWED(identifier, value) CECS_UNION_CREATE(CECS_BORROWED(identifier), identifier, value)
#define CECS_COW_CREATE_BORROWED_STRUCT(identifier, value) ((struct CECS_COW(identifier))CECS_COW_CREATE_BORROWED(identifier, value))

#define CECS_COW_IS_OWNED(identifier, cow) ((cow).variant == CECS_UNION_VARIANT(CECS_OWNED(identifier), identifier))
#define CECS_COW_IS_BORROWED(identifier, cow) ((cow).variant == CECS_UNION_VARIANT(CECS_BORROWED(identifier), identifier))
#define CECS_COW_IS_OWNED_ASSERT(identifier, cow) (assert(CECS_COW_IS_OWNED(identifier, cow) && "Invalid cow access"))
#define CECS_COW_IS_BORROWED_ASSERT(identifier, cow) (assert(CECS_COW_IS_BORROWED(identifier, cow) && "Invalid cow access"))

#define CECS_COW_GET_OWNED_UNCHECKED(identifier, cow) ((cow).CECS_UNION_VALUE(CECS_OWNED(identifier)))
#define CECS_COW_GET_BORROWED_UNCHECKED(identifier, cow) ((cow).CECS_UNION_VALUE(CECS_BORROWED(identifier)))
#define CECS_COW_GET_OWNED(identifier, cow) (CECS_COW_IS_OWNED_ASSERT(identifier, cow), CECS_COW_GET_OWNED_UNCHECKED(identifier, cow))
#define CECS_COW_GET_BORROWED(identifier, cow) (CECS_COW_IS_BORROWED_ASSERT(identifier, cow), CECS_COW_GET_BORROWED_UNCHECKED(identifier, cow))

#define CECS_COW_GET_REFERENCE(identifier, cow) \
    (CECS_COW_IS_OWNED(identifier, cow) \
        ? &CECS_COW_GET_OWNED_UNCHECKED(identifier, cow) \
        : CECS_COW_GET_BORROWED_UNCHECKED(identifier, cow))

#endif