#include "cecs_identifier.h"
#include <cecs_error.h>
#include <stdbool.h>

extern inline cecs_identifier_index cecs_identifier_index_of(const cecs_identifier identifier);
extern inline cecs_identifier_meta cecs_identifier_meta_of(const cecs_identifier identifier);

cecs_identifier cecs_identifier_from_value(const cecs_identifier_value value) {
    return (cecs_identifier){ .value = value };
}
cecs_identifier cecs_identifier_create_unchecked(const cecs_identifier_index index, const cecs_identifier_meta meta) {
    return cecs_identifier_from_value(
        ((cecs_identifier_value)index << (cecs_identifier_value)(CECS_IDENTIFIER_INDEX_BITS_OFFSET))
        | ((cecs_identifier_value)meta << (cecs_identifier_value)(CECS_IDENTIFIER_META_BITS_OFFSET))
    );
}
cecs_identifier cecs_identifier_create(const cecs_identifier_index index, const cecs_identifier_meta meta) {
#if (CECS_IDENTIFIER_INDEX_BITS_MASK >> CECS_IDENTIFIER_INDEX_BITS_OFFSET) < CECS_IDENTIFIER_INDEX_TYPE_MAX
    cecs_debugbreak_fail_unless(
        index <= (cecs_identifier_index)(CECS_IDENTIFIER_INDEX_BITS_MASK >> CECS_IDENTIFIER_INDEX_BITS_OFFSET),
        "error: cecs_identifier_create called with out of bounds index"
    );
#endif

#if (CECS_IDENTIFIER_META_BITS_MASK >> CECS_IDENTIFIER_META_BITS_OFFSET) < CECS_IDENTIFIER_META_TYPE_MAX
    cecs_debugbreak_fail_unless(
        meta <= (cecs_identifier_meta)(CECS_IDENTIFIER_META_BITS_MASK >> CECS_IDENTIFIER_META_BITS_OFFSET),
        "error: cecs_identifier_create called with out of bounds meta"
    );
#endif

    return cecs_identifier_create_unchecked(index, meta);
}

extern inline bool cecs_identifier_is_free(const cecs_identifier identifier, const size_t expected_index);
extern inline bool cecs_identifier_is_used(const cecs_identifier identifier, const size_t expected_index);
