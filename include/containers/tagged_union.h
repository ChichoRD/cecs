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
#define _TAGGED_UNION_VARIANT_FIELD(prefix, identifier) CAT3(prefix, _, identifier)
#define _TAGGED_UNION_STRUCT(identifier, prefix, ...) \
    struct identifier { \
        union { \
            MAP_PAIRS(_TAGGED_UNION_FIELD, EMPTY, __VA_ARGS__) \
        }; \
        enum { \
            MAP_CONST1(_TAGGED_UNION_VARIANT_FIELD, prefix, COMMA, MAP_PAIRS(_PREPEND_VARIANT_SELECT, COMMA, __VA_ARGS__)), \
        } variant; \
    }

#define TAGGED_UNION_STRUCT(prefix, ...) _TAGGED_UNION_STRUCT(TAGGED_UNION(__VA_ARGS__), prefix, __VA_ARGS__)

#define TAGGED_UNION_VALUE(identifier) _PREPEND_VALUE(identifier)
#define TAGGED_UNION_VARIANT(identifier, prefix) _TAGGED_UNION_VARIANT_FIELD(prefix, _PREPEND_VARIANT(identifier))
#define TAGGED_UNION_CREATE(identifier, prefix, value) \
    { .TAGGED_UNION_VALUE(identifier) = (value), .variant = TAGGED_UNION_VARIANT(identifier, prefix) }
#define TAGGED_UNION_STRUCT_CREATE(identifier, prefix, value, ...) \
    (TAGGED_UNION_STRUCT(__VA_ARGS__)) TAGGED_UNION_CREATE(identifier, prefix, (value))

#define TAGGED_UNION_CREATE_VARIANT(identifier, variant_value, value) \
    { .TAGGED_UNION_VALUE(identifier) = (value), .variant = (variant_value) }
#define TAGGED_UNION_STRUCT_CREATE_VARIANT(identifier, variant_value, value, ...) \
    (TAGGED_UNION_STRUCT(__VA_ARGS__)) TAGGED_UNION_CREATE_VARIANT(identifier, (variant_value), (value))

#define TAGGED_UNION_IS(identifier, prefix, union) ((union).variant == TAGGED_UNION_VARIANT(identifier, prefix))
#define TAGGED_UNION_IS_ASSERT(identifier, prefix, union) (assert(TAGGED_UNION_IS(identifier, prefix, (union)) && "Invalid union access"))
#define TAGGED_UNION_GET_UNCHECKED(identifier, union) ((union).TAGGED_UNION_VALUE(identifier))
#define TAGGED_UNION_GET(identifier, prefix, union) \
    (TAGGED_UNION_IS_ASSERT(identifier, prefix, (union)), TAGGED_UNION_GET_UNCHECKED(identifier, (union)))

#define TAGGED_UNION_MATCH(union) switch ((union).variant)


typedef uint8_t none;
#define NONE ((none)0)
#define OPTION(identifier) option_##identifier
#define OPTION_STRUCT(type, identifier) _TAGGED_UNION_STRUCT(OPTION(identifier), identifier, none, none, type, identifier)

#define OPTION_CREATE_SOME(identifier, value) TAGGED_UNION_CREATE(identifier, identifier, value)
#define OPTION_CREATE_NONE(identifier) TAGGED_UNION_CREATE(none, identifier, NONE)

#define OPTION_CREATE_SOME_STRUCT(identifier, value) ((struct OPTION(identifier))OPTION_CREATE_SOME(identifier, value))
#define OPTION_CREATE_NONE_STRUCT(identifier) ((struct OPTION(identifier))OPTION_CREATE_NONE(identifier))

#define OPTION_IS_NONE(identifier, option) ((option).variant == TAGGED_UNION_VARIANT(none, identifier))
#define OPTION_IS_SOME(identifier, option) ((option).variant != TAGGED_UNION_VARIANT(none, identifier))
#define OPTION_IS_NONE_ASSERT(identifier, option) (assert(OPTION_IS_NONE(identifier, option) && "Invalid option access"))
#define OPTION_IS_SOME_ASSERT(identifier, option) (assert(OPTION_IS_SOME(identifier, option) && "Invalid option access"))
#define OPTION_GET_UNCHECKED(identifier, option) ((option).TAGGED_UNION_VALUE(identifier))
#define OPTION_GET(identifier, option) (OPTION_IS_SOME_ASSERT(identifier, option), OPTION_GET_UNCHECKED(identifier, option))

#define OPTION_MAP(identifier, option, some_predicate, none_predicate) \
    (OPTION_IS_SOME(identifier, option) ? (some_predicate) : (none_predicate))
#define OPTION_GET_OR_NULL(identifier, option) \
    OPTION_MAP(identifier, option, OPTION_GET_UNCHECKED(identifier, option), NULL)
    
#define OPTION_MAP_REFERENCE(identifier, option, other_identifier) \
    OPTION_MAP( \
        identifier, \
        option, \
        OPTION_CREATE_SOME(other_identifier, OPTION_GET_UNCHECKED(identifier, option)), \
        OPTION_CREATE_NONE(other_identifier) \
    )
#define OPTION_MAP_REFERENCE_STRUCT(identifier, option, other_identifier) \
    OPTION_MAP( \
        identifier, \
        option, \
        OPTION_CREATE_SOME_STRUCT(other_identifier, OPTION_GET_UNCHECKED(identifier, option)), \
        OPTION_CREATE_NONE_STRUCT(other_identifier) \
    )

#define BORROWED(identifier) borrowed_##identifier
#define OWNED(identifier) owned_##identifier
#define COW(identifier) cow_##identifier
#define COW_STRUCT(type, identifier) \
    _TAGGED_UNION_STRUCT(COW(identifier), identifier, type *, BORROWED(identifier), type, OWNED(identifier))

#define COW_CREATE_OWNED(identifier, value) TAGGED_UNION_CREATE(OWNED(identifier), identifier, value)
#define COW_CREATE_OWNED_STRUCT(identifier, value) ((struct COW(identifier))COW_CREATE_OWNED(identifier, value))

#define COW_CREATE_BORROWED(identifier, value) TAGGED_UNION_CREATE(BORROWED(identifier), identifier, value)
#define COW_CREATE_BORROWED_STRUCT(identifier, value) ((struct COW(identifier))COW_CREATE_BORROWED(identifier, value))

#define COW_IS_OWNED(identifier, cow) ((cow).variant == TAGGED_UNION_VARIANT(OWNED(identifier), identifier))
#define COW_IS_BORROWED(identifier, cow) ((cow).variant == TAGGED_UNION_VARIANT(BORROWED(identifier), identifier))
#define COW_IS_OWNED_ASSERT(identifier, cow) (assert(COW_IS_OWNED(identifier, cow) && "Invalid cow access"))
#define COW_IS_BORROWED_ASSERT(identifier, cow) (assert(COW_IS_BORROWED(identifier, cow) && "Invalid cow access"))

#define COW_GET_OWNED_UNCHECKED(identifier, cow) ((cow).TAGGED_UNION_VALUE(OWNED(identifier)))
#define COW_GET_BORROWED_UNCHECKED(identifier, cow) ((cow).TAGGED_UNION_VALUE(BORROWED(identifier)))
#define COW_GET_OWNED(identifier, cow) (COW_IS_OWNED_ASSERT(identifier, cow), COW_GET_OWNED_UNCHECKED(identifier, cow))
#define COW_GET_BORROWED(identifier, cow) (COW_IS_BORROWED_ASSERT(identifier, cow), COW_GET_BORROWED_UNCHECKED(identifier, cow))

#define COW_GET_REFERENCE(identifier, cow) \
    (COW_IS_OWNED(identifier, cow) \
        ? &COW_GET_OWNED_UNCHECKED(identifier, cow) \
        : COW_GET_BORROWED_UNCHECKED(identifier, cow))

#endif