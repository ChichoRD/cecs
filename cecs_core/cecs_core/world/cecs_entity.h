#ifndef CECS_ENTITY_H
#define CECS_ENTITY_H

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#ifndef CECS_ENTITY_VALUE_TYPE
#define CECS_ENTITY_VALUE_TYPE_DEFAULT uint64_t
#define CECS_ENTITY_VALUE_TYPE CECS_ENTITY_VALUE_TYPE_DEFAULT

#define CECS_ENTITY_VALUE_TYPE_BITS_LOG2 6ull
#define CECS_ENTITY_VALUE_TYPE_BITS (1ull << CECS_ENTITY_VALUE_TYPE_BITS_LOG2)
#endif

#ifndef CECS_ENTITY_VALUE_TYPE_BITS_LOG2
static_assert(false, "static error: CECS_ENTITY_VALUE_TYPE_BITS_LOG2 must be defined");
#endif


typedef CECS_ENTITY_VALUE_TYPE cecs_entity_value;
typedef struct cecs_entity {
    cecs_entity_value value;
} cecs_entity;
inline cecs_entity cecs_entity_from_value_unchecked(const cecs_entity_value value) {
    return (cecs_entity){ .value = value };
}

#define CECS_ENTITY_META_BITS_LOG2 (CECS_ENTITY_VALUE_TYPE_BITS_LOG2 - 2ull)
#define CECS_ENTITY_META_BITS (1ull << CECS_ENTITY_META_BITS_LOG2)

#define CECS_ENTITY_INDEX_BITS (CECS_ENTITY_VALUE_TYPE_BITS - CECS_ENTITY_META_BITS)
#define CECS_ENTITY_INDEX_BITS_OFFSET (0ull)
#define CECS_ENTITY_INDEX_BITS_MASK (((1ull << (CECS_ENTITY_INDEX_BITS)) - 1ull) << (CECS_ENTITY_INDEX_BITS_OFFSET))

#define CECS_ENTITY_META_BITS_OFFSET (CECS_ENTITY_INDEX_BITS_OFFSET + CECS_ENTITY_INDEX_BITS)
static_assert(
    CECS_ENTITY_META_BITS + CECS_ENTITY_INDEX_BITS == CECS_ENTITY_VALUE_TYPE_BITS,
    "static error: CECS_ENTITY_META_BITS and CECS_ENTITY_INDEX_BITS must sum to CECS_ENTITY_VALUE_TYPE_BITS"
);
inline size_t cecs_entity_index(const cecs_entity entity) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - CECS_ENTITY_INDEX_BITS_OFFSET <= 64ull,
        "static error: cecs_entity_index only supports up to 64 bits of index"
    );
    return (size_t)(entity.value & (cecs_entity_value)CECS_ENTITY_INDEX_BITS_MASK);
}
inline uint_fast16_t cecs_entity_meta(const cecs_entity entity) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - CECS_ENTITY_META_BITS_OFFSET <= 16ull,
        "static error: cecs_entity_meta only supports up to 16 bits of index"
    );
    return (uint_fast16_t)(entity.value >> (cecs_entity_value)(CECS_ENTITY_META_BITS_OFFSET));
}


#define CECS_ENTITY_GENERAL_META_BITS_LOG2 (CECS_ENTITY_META_BITS_LOG2 - 1ull)
#define CECS_ENTITY_GENERAL_META_BITS (1ull << CECS_ENTITY_GENERAL_META_BITS_LOG2)
#define CECS_ENTITY_GENERAL_META_BITS_OFFSET (CECS_ENTITY_META_BITS_OFFSET)
#define CECS_ENTITY_GENERAL_META_BITS_MASK (((1ull << (CECS_ENTITY_GENERAL_META_BITS)) - 1ull) << (CECS_ENTITY_GENERAL_META_BITS_OFFSET))

#define CECS_ENTITY_GENERATION_BITS_LOG2 (CECS_ENTITY_META_BITS_LOG2 - 1ull)
#define CECS_ENTITY_GENERATION_BITS (1ull << CECS_ENTITY_GENERATION_BITS_LOG2)
#define CECS_ENTITY_GENERATION_BITS_OFFSET (CECS_ENTITY_GENERAL_META_BITS_OFFSET + CECS_ENTITY_GENERAL_META_BITS)
#define CECS_ENTITY_GENERATION_BITS_MASK (((1ull << (CECS_ENTITY_GENERATION_BITS)) - 1ull) << (CECS_ENTITY_GENERATION_BITS_OFFSET))
static_assert(
    CECS_ENTITY_GENERATION_BITS + CECS_ENTITY_GENERAL_META_BITS == CECS_ENTITY_META_BITS,
    "static error: CECS_ENTITY_GENERATION_BITS and CECS_ENTITY_GENERAL_META_BITS must sum to CECS_ENTITY_META_BITS"
);
inline uint_fast8_t cecs_entity_generation(const cecs_entity entity) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - (CECS_ENTITY_GENERATION_BITS_OFFSET) <= 8ull,
        "static error: cecs_entity_generation only supports up to 8 bits of index"
    );
    return (uint_fast8_t)(entity.value >> (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET));
}
inline cecs_entity cecs_entity_set_generation(const cecs_entity entity, const uint_fast8_t generation) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - (CECS_ENTITY_GENERATION_BITS_OFFSET) <= 8ull,
        "static error: cecs_entity_set_generation only supports up to 8 bits of index"
    );
    return cecs_entity_from_value_unchecked(
        (entity.value & ~((cecs_entity_value)CECS_ENTITY_GENERATION_BITS_MASK))
        | ((cecs_entity_value)generation << (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET))
    );
}
inline cecs_entity cecs_entity_generation_add(const cecs_entity entity, const uint_fast8_t generation_add) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - (CECS_ENTITY_GENERATION_BITS_OFFSET) <= 8ull,
        "static error: cecs_entity_generation_add only supports up to 8 bits of index"
    );
    const cecs_entity_value add_value = (cecs_entity_value)generation_add << (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET);
    return cecs_entity_from_value_unchecked(entity.value + add_value);
}
inline cecs_entity cecs_entity_generation_sub(const cecs_entity entity, const uint_fast8_t generation_sub) {
    static_assert(
        CECS_ENTITY_VALUE_TYPE_BITS - (CECS_ENTITY_GENERATION_BITS_OFFSET) <= 8ull,
        "static error: cecs_entity_generation_inc only supports up to 8 bits of index"
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
    cecs_entity_meta_type_alive = 1 << 1,
    // TODO: more flags
} cecs_entity_meta_type;
typedef uint8_t cecs_entity_meta_flag;
inline cecs_entity_meta_flag cecs_entity_meta_flags(const cecs_entity entity) {
    static_assert(
        CECS_ENTITY_GENERAL_META_BITS <= 8ull,
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

inline bool cecs_entity_is_alive(const cecs_entity entity) {
    return cecs_entity_has_meta_flag(entity, cecs_entity_meta_type_alive);
}
inline cecs_entity cecs_entity_set_alive(const cecs_entity entity) {
    return cecs_entity_set_meta_flag(entity, cecs_entity_meta_type_alive);
}
inline cecs_entity cecs_entity_unset_alive(const cecs_entity entity) {
    return cecs_entity_unset_meta_flag(entity, cecs_entity_meta_type_alive);
}

cecs_entity cecs_entity_from_value(const cecs_entity_value value);
cecs_entity cecs_entity_create_unchecked(const size_t index, const cecs_entity_meta_flag flags, const uint_fast8_t generation);
cecs_entity cecs_entity_create(const size_t index, const cecs_entity_meta_flag flags, const uint_fast8_t generation);
cecs_entity cecs_entity_create_expect(const size_t index, const cecs_entity_meta_flag flags, const uint_fast8_t generation);

#endif
