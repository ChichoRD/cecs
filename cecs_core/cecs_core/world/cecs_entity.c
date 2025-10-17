#include "cecs_entity.h"
#include <cecs_core/cecs_error.h>

extern inline cecs_entity cecs_entity_from_value_unchecked(const cecs_entity_value value);

extern inline size_t cecs_entity_index(const cecs_entity entity);
extern inline uint_fast16_t cecs_entity_meta(const cecs_entity entity);

extern inline uint_fast8_t cecs_entity_generation(const cecs_entity entity);
extern inline cecs_entity cecs_entity_set_generation(const cecs_entity entity, const uint_fast8_t generation);
extern inline cecs_entity cecs_entity_generation_add(const cecs_entity entity, const uint_fast8_t generation_add);
extern inline cecs_entity cecs_entity_generation_sub(const cecs_entity entity, const uint_fast8_t generation_sub);
extern inline cecs_entity cecs_entity_next_generation(const cecs_entity entity);
extern inline cecs_entity cecs_entity_prev_generation(const cecs_entity entity);

extern inline cecs_entity_meta_flag cecs_entity_meta_flags(const cecs_entity entity);
extern inline bool cecs_entity_has_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag);
extern inline cecs_entity cecs_entity_set_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag);
extern inline cecs_entity cecs_entity_unset_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag);

extern inline bool cecs_entity_is_illegal(const cecs_entity entity);
extern inline cecs_entity cecs_entity_set_illegal(const cecs_entity entity);
extern inline cecs_entity cecs_entity_unset_illegal(const cecs_entity entity);

extern inline bool cecs_entity_is_alive(const cecs_entity entity);
extern inline cecs_entity cecs_entity_set_alive(const cecs_entity entity);
extern inline cecs_entity cecs_entity_unset_alive(const cecs_entity entity);

cecs_entity cecs_entity_from_value(const cecs_entity_value value) {
    const cecs_entity entity = cecs_entity_from_value_unchecked(value);
    cecs_assert_or_exit(
        !cecs_entity_is_illegal(entity),
        "error: cecs_entity_from_value called with illegal entity value"
    );
    return entity;
}
extern inline cecs_entity cecs_entity_create_unchecked(const size_t index, const cecs_entity_meta_flag flags, const uint_fast8_t generation) {
    return cecs_entity_from_value_unchecked(
        ((cecs_entity_value)index << (cecs_entity_value)(CECS_ENTITY_INDEX_BITS_OFFSET))
        | ((cecs_entity_value)flags << (cecs_entity_value)(CECS_ENTITY_GENERAL_META_BITS_OFFSET))
        | ((cecs_entity_value)generation << (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET))
    );
}
cecs_entity cecs_entity_create(const size_t index, const cecs_entity_meta_flag flags, const uint_fast8_t generation) {
    cecs_assert_or_exit(
        index <= (size_t)CECS_ENTITY_INDEX_BITS_MASK,
        "error: cecs_entity_create called with out of bounds index"
    );
    cecs_assert_or_exit(
        generation <= (uint_fast8_t)(CECS_ENTITY_GENERATION_BITS_MASK >> CECS_ENTITY_GENERATION_BITS_OFFSET),
        "error: cecs_entity_create called with out of bounds generation"
    );
    return cecs_entity_create_unchecked(index, flags, generation);
}
cecs_entity cecs_entity_create_expect(const size_t index, const cecs_entity_meta_flag flags, const uint_fast8_t generation) {
    cecs_assert_or_exit(
        !(flags & cecs_entity_meta_type_illegal),
        "error: cecs_entity_create_expect called with illegal flags"
    );
    return cecs_entity_create(index, flags, generation);
}
