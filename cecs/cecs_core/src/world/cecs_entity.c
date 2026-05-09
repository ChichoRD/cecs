#include "cecs_entity.h"
#include <cecs_error.h>
#include <stdint.h>


extern inline cecs_entity_index cecs_entity_index_of(const cecs_entity entity);
extern inline cecs_entity_meta cecs_entity_meta_of(const cecs_entity entity);


extern inline cecs_entity_generation cecs_entity_generation_of(const cecs_entity entity);
extern inline cecs_entity cecs_entity_set_generation(const cecs_entity entity, const cecs_entity_generation generation);
extern inline cecs_entity cecs_entity_generation_add(const cecs_entity entity, const cecs_entity_generation generation_add);
extern inline cecs_entity cecs_entity_generation_sub(const cecs_entity entity, const cecs_entity_generation generation_sub);
extern inline cecs_entity cecs_entity_next_generation(const cecs_entity entity);
extern inline cecs_entity cecs_entity_prev_generation(const cecs_entity entity);


extern inline cecs_entity_meta_flag cecs_entity_meta_flags(const cecs_entity entity);
extern inline bool cecs_entity_has_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag);
extern inline cecs_entity cecs_entity_set_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag);
extern inline cecs_entity cecs_entity_unset_meta_flag(const cecs_entity entity, const cecs_entity_meta_flag flag);

extern inline bool cecs_entity_is_illegal(const cecs_entity entity);
extern inline cecs_entity cecs_entity_set_illegal(const cecs_entity entity);
extern inline cecs_entity cecs_entity_unset_illegal(const cecs_entity entity);

extern inline cecs_entity cecs_entity_from_value_unchecked(const cecs_entity_value value);
cecs_entity cecs_entity_from_value(const cecs_entity_value value) {
    const cecs_entity entity = cecs_entity_from_value_unchecked(value);
    cecs_debugbreak_fail_unless(
        !cecs_entity_is_illegal(entity),
        "error: cecs_entity_from_value called with illegal entity value"
    );
    return entity;
}
extern inline cecs_entity cecs_entity_create_unchecked(const size_t index, const cecs_entity_meta_flag flags, const cecs_entity_generation generation) {
    return cecs_entity_from_value_unchecked(
        ((cecs_entity_value)index << (cecs_entity_value)(CECS_ENTITY_INDEX_BITS_OFFSET))
        | ((cecs_entity_value)flags << (cecs_entity_value)(CECS_ENTITY_GENERAL_META_BITS_OFFSET))
        | ((cecs_entity_value)generation << (cecs_entity_value)(CECS_ENTITY_GENERATION_BITS_OFFSET))
    );
}
cecs_entity cecs_entity_create(const size_t index, const cecs_entity_meta_flag flags, const cecs_entity_generation generation) {
    cecs_debugbreak_fail_unless(
        index <= (size_t)CECS_ENTITY_INDEX_BITS_MASK,
        "error: cecs_entity_create called with out of bounds index"
    );

#ifndef CECS_ENTITY_GENERATION_MAX
#define CECS_ENTITY_GENERATION_MAX (CECS_ENTITY_GENERATION_BITS_MASK >> CECS_ENTITY_GENERATION_BITS_OFFSET)
#endif
#if (CECS_ENTITY_GENERATION_MAX) < CECS_ENTITY_GENERATION_TYPE_MAX
    cecs_debugbreak_fail_unless(
        generation <= (cecs_entity_generation)(CECS_ENTITY_GENERATION_MAX),
        "error: cecs_entity_create called with out of bounds generation"
    );
#endif
#undef CECS_ENTITY_GENERATION_MAX
    return cecs_entity_create_unchecked(index, flags, generation);
}
