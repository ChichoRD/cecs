#ifndef CECS_ENTITY_H
#define CECS_ENTITY_H

#include "registry/cecs_identifier.h"

typedef cecs_identifier_value cecs_entity_value;
typedef cecs_identifier cecs_entity;


inline cecs_entity cecs_entity_from_value_unchecked(const cecs_entity_value value) {
    return cecs_identifier_from_value(value);
}

#define CECS_ENTITY_VALUE_TYPE_BITS CECS_IDENTIFIER_VALUE_TYPE_BITS

#define CECS_ENTITY_INDEX_BITS_LOG2     CECS_IDENTIFIER_INDEX_BITS_LOG2
#define CECS_ENTITY_INDEX_BITS          CECS_IDENTIFIER_INDEX_BITS
#define CECS_ENTITY_INDEX_BITS_OFFSET   CECS_IDENTIFIER_INDEX_BITS_OFFSET
#define CECS_ENTITY_INDEX_BITS_MASK     CECS_IDENTIFIER_INDEX_BITS_MASK

#define CECS_ENTITY_META_BITS_LOG2      CECS_IDENTIFIER_META_BITS_LOG2
#define CECS_ENTITY_META_BITS           CECS_IDENTIFIER_META_BITS
#define CECS_ENTITY_META_BITS_OFFSET    CECS_IDENTIFIER_META_BITS_OFFSET
#define CECS_ENTITY_META_BITS_MASK      CECS_IDENTIFIER_META_BITS_MASK

#define CECS_ENTITY_GENERAL_META_BITS_LOG2      (CECS_ENTITY_META_BITS_LOG2 - 1ull)
#define CECS_ENTITY_GENERAL_META_BITS           (1ull << CECS_ENTITY_GENERAL_META_BITS_LOG2)
#define CECS_ENTITY_GENERAL_META_BITS_OFFSET    CECS_IDENTIFIER_META_BITS_OFFSET
#define CECS_ENTITY_GENERAL_META_BITS_MASK      (((1ull << (CECS_ENTITY_GENERAL_META_BITS)) - 1ull) << (CECS_ENTITY_GENERAL_META_BITS_OFFSET))

#define CECS_ENTITY_GENERATION_BITS_LOG2    (CECS_ENTITY_META_BITS_LOG2 - 1ull)
#define CECS_ENTITY_GENERATION_BITS         (1ull << CECS_ENTITY_GENERATION_BITS_LOG2)
#define CECS_ENTITY_GENERATION_BITS_OFFSET  (CECS_ENTITY_GENERAL_META_BITS_OFFSET + CECS_ENTITY_GENERAL_META_BITS)
#define CECS_ENTITY_GENERATION_BITS_MASK    (((1ull << (CECS_ENTITY_GENERATION_BITS)) - 1ull) << (CECS_ENTITY_GENERATION_BITS_OFFSET))
static_assert(
    CECS_ENTITY_GENERATION_BITS + CECS_ENTITY_GENERAL_META_BITS == CECS_ENTITY_META_BITS,
    "static error: CECS_ENTITY_GENERATION_BITS and CECS_ENTITY_GENERAL_META_BITS must sum to CECS_ENTITY_META_BITS"
);


typedef size_t cecs_entity_index;
#define CECS_ENTITY_INDEX_TYPE_MAX SIZE_MAX
static_assert(
    (cecs_entity_index)(~((cecs_entity_index)0u)) == CECS_ENTITY_INDEX_TYPE_MAX,
    "static error: CECS_ENTITY_INDEX_TYPE_MAX does not match the maximum value of cecs_entity_index type"
);
static_assert(
    CECS_IDENTIFIER_INDEX_TYPE_MAX <= CECS_ENTITY_INDEX_TYPE_MAX,
    "static error: CECS_ENTITY_INDEX_TYPE_MAX must be less than or equal to CECS_ENTITY_INDEX_TYPE_MAX\n"
    "note: cecs_entity_index must be able to represent all possible identifier indices since entity indices are stored in the same bits as identifier indices"
);

inline cecs_entity_index cecs_entity_index_of(const cecs_entity entity) {
    return (cecs_entity_index)cecs_identifier_index_of(entity);
}


typedef uint_fast16_t cecs_entity_meta;
#define CECS_ENTITY_META_TYPE_MAX UINT_FAST16_MAX
static_assert(
    (cecs_entity_meta)(~((cecs_entity_meta)0u)) == CECS_ENTITY_META_TYPE_MAX,
    "static error: CECS_ENTITY_META_TYPE_MAX does not match the maximum value of cecs_entity_meta type"
);
static_assert(
    CECS_IDENTIFIER_META_TYPE_MAX <= CECS_ENTITY_META_TYPE_MAX,
    "static error: CECS_ENTITY_META_TYPE_MAX must be greater than or equal to CECS_IDENTIFIER_META_TYPE_MAX\n"
    "note: cecs_entity_meta must be able to represent all possible identifier meta values since entity meta values are stored in the same bits as identifier meta values"
);

inline cecs_entity_meta cecs_entity_meta_of(const cecs_entity entity) {
    return (cecs_entity_meta)cecs_identifier_meta_of(entity);
}


// TODO: let user define 'holder' type
typedef uint_fast8_t cecs_entity_generation;
// #define CECS_ENTITY_META_GENERATION_BITS_LOG2 (CECS_ENTITY_META_BITS_LOG2 - 1ull)
// #define CECS_ENTITY_META_GENERATION_BITS (1ull << CECS_ENTITY_META_GENERATION_BITS_LOG2)
#define CECS_ENTITY_GENERATION_TYPE_MAX UINT_FAST8_MAX
static_assert(
    (cecs_entity_generation)(~((cecs_entity_generation)0u)) == CECS_ENTITY_GENERATION_TYPE_MAX,
    "static error: CECS_ENTITY_GENERATION_TYPE_MAX does not match the maximum value of cecs_entity_generation type"
);
static_assert(
    CECS_ENTITY_GENERATION_BITS <= sizeof(cecs_entity_generation) * CHAR_BIT,
    "static error: cecs_entity_generation must be able to hold CECS_ENTITY_GENERATION_BITS number of bits to represent all entity generations"
);


inline cecs_entity_generation cecs_entity_generation_of(const cecs_entity entity) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - (CECS_ENTITY_GENERATION_BITS_OFFSET) <= 8ull,
        "static error: cecs_entity_generation only supports up to 8 bits of index"
    );
    return (cecs_entity_generation)(entity.value >> (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET));
}
inline cecs_entity cecs_entity_set_generation(const cecs_entity entity, const cecs_entity_generation generation) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - (CECS_ENTITY_GENERATION_BITS_OFFSET) <= 8ull,
        "static error: cecs_entity_set_generation only supports up to 8 bits of index"
    );
    return cecs_entity_from_value_unchecked(
        (entity.value & ~((cecs_entity_value)CECS_ENTITY_GENERATION_BITS_MASK))
        | ((cecs_entity_value)generation << (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET))
    );
}
inline cecs_entity cecs_entity_generation_add(const cecs_entity entity, const cecs_entity_generation generation_add) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - (CECS_ENTITY_GENERATION_BITS_OFFSET) <= 8ull,
        "static error: cecs_entity_generation_add only supports up to 8 bits of index"
    );
    const cecs_entity_value add_value = (cecs_entity_value)generation_add << (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET);
    return cecs_entity_from_value_unchecked(entity.value + add_value);
}
inline cecs_entity cecs_entity_generation_sub(const cecs_entity entity, const cecs_entity_generation generation_sub) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - (CECS_ENTITY_GENERATION_BITS_OFFSET) <= 8ull,
        "static error: cecs_entity_generation_sub only supports up to 8 bits of index"
    );
    const cecs_entity_value sub_value = (cecs_entity_value)generation_sub << (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET);
    return cecs_entity_from_value_unchecked(entity.value - sub_value);
}
inline cecs_entity cecs_entity_next_generation(const cecs_entity entity) {
    return cecs_entity_generation_add(entity, 1ull);
}
inline cecs_entity cecs_entity_prev_generation(const cecs_entity entity) {
    return cecs_entity_generation_sub(entity, 1ull);
}


typedef enum cecs_entity_meta_type {
    cecs_entity_meta_type_none = 0,
    cecs_entity_meta_type_illegal = 1 << 0,
    // cecs_entity_meta_type_free = 1 << 1, // not needed since the identifier allocator already tracks free identifiers
    // TODO: more flags
} cecs_entity_meta_type;
typedef uint8_t cecs_entity_meta_flag;
inline cecs_entity_meta_flag cecs_entity_meta_flags(const cecs_entity entity) {
    static_assert(
        CECS_ENTITY_GENERAL_META_BITS <= sizeof(cecs_entity_meta_flag) * CHAR_BIT,
        "static error: cecs_entity_meta_flags only supports up to 8 bits of index"
    );
    return (cecs_entity_meta_flag)(entity.value >> (cecs_entity_value)(CECS_ENTITY_GENERAL_META_BITS_OFFSET));
}
inline bool cecs_entity_has_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag) {
    return (entity.value & ((cecs_entity_value)flag << (cecs_entity_value)(CECS_ENTITY_GENERAL_META_BITS_OFFSET))) != 0;
}
inline cecs_entity cecs_entity_set_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag) {
    return cecs_entity_from_value_unchecked(
        entity.value | ((cecs_entity_value)flag << (cecs_entity_value)(CECS_ENTITY_GENERAL_META_BITS_OFFSET))
    );
}
inline cecs_entity cecs_entity_unset_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag) {
    return cecs_entity_from_value_unchecked(
        entity.value & ~((cecs_entity_value)flag << (cecs_entity_value)(CECS_ENTITY_GENERAL_META_BITS_OFFSET))
    );
}

inline bool cecs_entity_is_illegal(const cecs_entity entity) {
    return cecs_entity_has_meta_flag(entity, cecs_entity_meta_type_illegal);
}
inline cecs_entity cecs_entity_set_illegal(const cecs_entity entity) {
    return cecs_entity_set_meta_flag(entity, cecs_entity_meta_type_illegal);
}
inline cecs_entity cecs_entity_unset_illegal(const cecs_entity entity) {
    return cecs_entity_unset_meta_flag(entity, cecs_entity_meta_type_illegal);
}


cecs_entity cecs_entity_from_value(const cecs_entity_value value);
cecs_entity cecs_entity_create_unchecked(const size_t index, const cecs_entity_meta_flag flags, const cecs_entity_generation generation);
cecs_entity cecs_entity_create(const size_t index, const cecs_entity_meta_flag flags, const cecs_entity_generation generation);

#endif
