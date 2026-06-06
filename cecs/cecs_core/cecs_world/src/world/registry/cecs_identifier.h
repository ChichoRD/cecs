#ifndef CECS_IDENTIFIER_H
#define CECS_IDENTIFIER_H


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>

#include <cecs_platform.h>

#ifndef CECS_IDENTIFIER_VALUE_TYPE
#define CECS_IDENTIFIER_VALUE_TYPE_DEFAULT size_t
#define CECS_IDENTIFIER_VALUE_TYPE CECS_IDENTIFIER_VALUE_TYPE_DEFAULT

#define CECS_IDENTIFIER_VALUE_TYPE_BITS_LOG2 CECS_PLATFORM_WORD_BITS_LOG2
#endif


#ifndef CECS_IDENTIFIER_VALUE_TYPE_BITS_LOG2
#error "static error: CECS_IDENTIFIER_VALUE_TYPE_BITS_LOG2 must be defined"
#else

#define CECS_IDENTIFIER_VALUE_TYPE_BITS (1ull << CECS_IDENTIFIER_VALUE_TYPE_BITS_LOG2)
typedef CECS_IDENTIFIER_VALUE_TYPE cecs_identifier_value;
static_assert(
    sizeof(cecs_identifier_value) * CHAR_BIT == CECS_IDENTIFIER_VALUE_TYPE_BITS,
    "static error: CECS_IDENTIFIER_VALUE_TYPE_BITS must match the number of bits in CECS_IDENTIFIER_VALUE_TYPE"
);

#endif


typedef struct cecs_identifier {
    cecs_identifier_value value;
} cecs_identifier;

#if !defined(CECS_IDENTIFIER_INDEX_BITS) && !defined(CECS_IDENTIFIER_META_BITS)
#define CECS_IDENTIFIER_META_BITS_LOG2 (CECS_IDENTIFIER_VALUE_TYPE_BITS_LOG2 - 2ull)
#define CECS_IDENTIFIER_META_BITS (1ull << CECS_IDENTIFIER_META_BITS_LOG2)

#define CECS_IDENTIFIER_INDEX_BITS (CECS_IDENTIFIER_VALUE_TYPE_BITS - CECS_IDENTIFIER_META_BITS)
#define CECS_IDENTIFIER_INDEX_BITS_OFFSET (0ull)

#define CECS_IDENTIFIER_META_BITS_OFFSET (CECS_IDENTIFIER_INDEX_BITS_OFFSET + CECS_IDENTIFIER_INDEX_BITS)
#endif


#ifndef CECS_IDENTIFIER_META_BITS
#error "static error: CECS_IDENTIFIER_META_BITS must be defined"
#endif
#ifndef CECS_IDENTIFIER_INDEX_BITS
#error "static error: CECS_IDENTIFIER_INDEX_BITS must be defined"
#endif
#ifndef CECS_IDENTIFIER_META_BITS_OFFSET
#error "static error: CECS_IDENTIFIER_META_BITS_OFFSET must be defined"
#endif
#ifndef CECS_IDENTIFIER_INDEX_BITS_OFFSET
#error "static error: CECS_IDENTIFIER_INDEX_BITS_OFFSET must be defined"
#endif

#define CECS_IDENTIFIER_INDEX_BITS_MASK (((1ull << (CECS_IDENTIFIER_INDEX_BITS)) - 1ull) << (CECS_IDENTIFIER_INDEX_BITS_OFFSET))
#define CECS_IDENTIFIER_META_BITS_MASK (((1ull << (CECS_IDENTIFIER_META_BITS)) - 1ull) << (CECS_IDENTIFIER_META_BITS_OFFSET))
#define CECS_IDENTIFIER_INDEX_MAX (CECS_IDENTIFIER_INDEX_BITS_MASK >> CECS_IDENTIFIER_INDEX_BITS_OFFSET)
#define CECS_IDENTIFIER_META_MAX (CECS_IDENTIFIER_META_BITS_MASK >> CECS_IDENTIFIER_META_BITS_OFFSET)


#ifndef CECS_IDENTIFIER_INDEX_TYPE
#define CECS_IDENTIFIER_INDEX_TYPE_DEFAULT size_t
#define CECS_IDENTIFIER_INDEX_TYPE CECS_IDENTIFIER_INDEX_TYPE_DEFAULT
#define CECS_IDENTIFIER_INDEX_TYPE_MAX SIZE_MAX
#endif
#ifndef CECS_IDENTIFIER_META_TYPE
#define CECS_IDENTIFIER_META_TYPE_DEFAULT uint16_t
#define CECS_IDENTIFIER_META_TYPE CECS_IDENTIFIER_META_TYPE_DEFAULT
#define CECS_IDENTIFIER_META_TYPE_MAX UINT16_MAX
#endif

#ifndef CECS_IDENTIFIER_INDEX_TYPE
#error "static error: CECS_IDENTIFIER_INDEX_TYPE must be defined"
#endif
#ifndef CECS_IDENTIFIER_META_TYPE
#error "static error: CECS_IDENTIFIER_META_TYPE must be defined"
#endif
#ifndef CECS_IDENTIFIER_INDEX_TYPE_MAX
#error "static error: CECS_IDENTIFIER_INDEX_TYPE_MAX must be defined"
#endif
#ifndef CECS_IDENTIFIER_META_TYPE_MAX
#error "static error: CECS_IDENTIFIER_META_TYPE_MAX must be defined"
#endif

typedef CECS_IDENTIFIER_INDEX_TYPE cecs_identifier_index;
typedef CECS_IDENTIFIER_META_TYPE cecs_identifier_meta;

static_assert(
    (cecs_identifier_index)(~((cecs_identifier_index)0u)) == CECS_IDENTIFIER_INDEX_TYPE_MAX,
    "static error: CECS_IDENTIFIER_INDEX_TYPE_MAX does not match the maximum value of cecs_identifier_index"
);
static_assert(
    (cecs_identifier_meta)(~((cecs_identifier_meta)0u)) == CECS_IDENTIFIER_META_TYPE_MAX,
    "static error: CECS_IDENTIFIER_META_TYPE_MAX does not match the maximum value of cecs_identifier_meta"
);

static_assert(
    CECS_IDENTIFIER_INDEX_BITS <= sizeof(cecs_identifier_index) * CHAR_BIT,
    "static error: CECS_IDENTIFIER_INDEX_BITS must be less than or equal to the number of bits in CECS_IDENTIFIER_INDEX_TYPE"
);
static_assert(
    CECS_IDENTIFIER_META_BITS <= sizeof(cecs_identifier_meta) * CHAR_BIT,
    "static error: CECS_IDENTIFIER_META_BITS must be less than or equal to the number of bits in CECS_IDENTIFIER_META_TYPE"
);


static_assert(
    CECS_IDENTIFIER_META_BITS + CECS_IDENTIFIER_INDEX_BITS == CECS_IDENTIFIER_VALUE_TYPE_BITS,
    "static error: CECS_IDENTIFIER_META_BITS and CECS_IDENTIFIER_INDEX_BITS must sum to CECS_IDENTIFIER_VALUE_TYPE_BITS"
);
inline cecs_identifier_index cecs_identifier_index_of(const cecs_identifier identifier) {
    static_assert(
        CECS_IDENTIFIER_VALUE_TYPE_BITS - CECS_IDENTIFIER_INDEX_BITS_OFFSET <= sizeof(cecs_identifier_index) * CHAR_BIT,
        "static error: cecs_identifier_index only supports up to (sizeof(cecs_identifier_index) * CHAR_BIT) bits of index"
    );
    return (cecs_identifier_index)((identifier.value
            & (cecs_identifier_value)CECS_IDENTIFIER_INDEX_BITS_MASK)
                >> (cecs_identifier_value)(CECS_IDENTIFIER_INDEX_BITS_OFFSET));
}
inline cecs_identifier_meta cecs_identifier_meta_of(const cecs_identifier identifier) {
    static_assert(
        CECS_IDENTIFIER_VALUE_TYPE_BITS - CECS_IDENTIFIER_META_BITS_OFFSET <= sizeof(cecs_identifier_meta) * CHAR_BIT,
        "static error: cecs_identifier_meta only supports up to (sizeof(cecs_identifier_meta) * CHAR_BIT) bits of meta"
    );
    return (cecs_identifier_meta)((identifier.value
            & (cecs_identifier_value)CECS_IDENTIFIER_META_BITS_MASK)
                >> (cecs_identifier_value)(CECS_IDENTIFIER_META_BITS_OFFSET));
}


cecs_identifier cecs_identifier_from_value(const cecs_identifier_value value);
cecs_identifier cecs_identifier_create_unchecked(const cecs_identifier_index index, const cecs_identifier_meta meta);
cecs_identifier cecs_identifier_create(const cecs_identifier_index index, const cecs_identifier_meta meta);

inline bool cecs_identifier_is_free(const cecs_identifier identifier, const size_t expected_index) {
    return cecs_identifier_index_of(identifier) != expected_index;
}
inline bool cecs_identifier_is_used(const cecs_identifier identifier, const size_t expected_index) {
    return cecs_identifier_index_of(identifier) == expected_index;
}

#endif
