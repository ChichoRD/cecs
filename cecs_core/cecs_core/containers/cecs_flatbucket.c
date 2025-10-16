#include "cecs_flatbucket.h"
#include <string.h>
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>

const uint_fast8_t cecs_flatbucket_max_count = CECS_FLATBUCKET8_MAX_COUNT;

#define CECS_FLATBUCKET8_HASH7_MAX_LOG2 7
#define CECS_FLATBUCKET8_HASH7_MAX (1 << CECS_FLATBUCKET8_HASH7_MAX_LOG2)
#define CECS_FLATBUCKET8_HASH7_MASK (CECS_FLATBUCKET8_HASH7_MAX - 1)

static const cecs_flatbucket_hash_low_fast cecs_flatbucket_hash7_max = CECS_FLATBUCKET8_HASH7_MAX;
cecs_flatbucket_hash_low_fast cecs_flatbucket_hash_low_get(const cecs_flatbucket_hash hash) {
    return (cecs_flatbucket_hash_low_fast)(hash & CECS_FLATBUCKET8_HASH7_MASK);
}

extern inline const void *cecs_flatbucket_get_value_unchecked(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t offset);
extern inline void *cecs_flatbucket_get_value_mut_unchecked(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t offset);
extern inline const void *cecs_flatbucket_get_value(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t offset);
extern inline void *cecs_flatbucket_get_value_mut(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t offset);

extern inline uint_fast8_t cecs_flatbucket_get_count(const cecs_flatbucket bucket);
extern inline bool cecs_flatbucket_is_full(const cecs_flatbucket bucket);

static inline cecs_flatbucket_hash_low_fast cecs_flatbucket_get_hash_low(const cecs_flatbucket bucket, const uint_fast8_t index) {
    if (index >= cecs_flatbucket_max_count) {
        assert(false && "error: cecs_flatbucket_get_hash_low called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return (bucket.hash_from_index8_u7 >> (index * 7 + 8)) & 0x7Full;
    // (index * 7 + 8)
    // (index * (8 - 1) + 8)
    // (index * 8 - index + 8)
    // ((index + 1) * 8 - index)
}
static inline void cecs_flatbucket_set_hash_low(cecs_flatbucket *bucket, const uint_fast8_t index, const cecs_flatbucket_hash_low_fast hash7) {
    if (index >= cecs_flatbucket_max_count) {
        assert(false && "error: cecs_flatbucket_set_hash_low called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    if (hash7 >= cecs_flatbucket_hash7_max) {
        assert(false && "error: cecs_flatbucket_set_hash_low called with out of bounds hash7");
        exit(EXIT_FAILURE);
    }
    bucket->hash_from_index8_u7 &= ~(0x7Full << (index * 7 + 8));
    bucket->hash_from_index8_u7 |= (hash7 & 0x7Full) << (index * 7 + 8);
}

extern inline bool cecs_flatbucket_has_been_full(const cecs_flatbucket bucket);
inline void cecs_flatbucket_set_been_full(cecs_flatbucket *bucket) {
    bucket->hash_from_index8_u7 |= 0x80ull;
}
inline void cecs_flatbucket_unset_been_full(cecs_flatbucket *bucket) {
    bucket->hash_from_index8_u7 &= ~0x80ull;
}
static inline void cecs_flatbucket_set_been_full_if_full(cecs_flatbucket *bucket) {
    bucket->hash_from_index8_u7 |= cecs_flatbucket_get_count(*bucket) << 4;
}
inline void cecs_flatbucket_unset_been_full_if_not_full(cecs_flatbucket *bucket) {
    bucket->hash_from_index8_u7 &= cecs_flatbucket_get_count(*bucket) << 4;
}

static inline void *cecs_flatbucket_push(cecs_flatbucket *bucket, const size_t value_size, const size_t values_offset) {
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*bucket);
    if (bucket_value_count > cecs_flatbucket_max_count) {
        assert(false && "error: cecs_flatbucket_push called with full bucket");
        exit(EXIT_FAILURE);
    }
    void *const value = &bucket->values[bucket_value_count * value_size + values_offset];
    ++bucket->hash_from_index8_u7;
    return value;
}
void *cecs_flatbucket_insert_expect(
    cecs_flatbucket *bucket,
    const uint_fast8_t index,
    const cecs_flatbucket_hash_low hash7,
    const size_t value_size,
    const size_t values_offset
) {
    if (cecs_flatbucket_is_full(*bucket)) {
        assert(false && "error: cecs_flatbucket_insert_expect called with full bucket");
        exit(EXIT_FAILURE);
    }
    
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*bucket);
    if (index != bucket_value_count) {
        assert(false && "error: cecs_flatbucket_insert_expect called with already occupied hash_low");
        exit(EXIT_FAILURE);
    } else {
        cecs_flatbucket_set_hash_low(bucket, index, hash7);
        void *const value = cecs_flatbucket_push(bucket, value_size, values_offset);
        cecs_flatbucket_set_been_full_if_full(bucket);
        return value;
    }
}

static inline void cecs_flatbucket_swap_last_pop(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t values_offset) {
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*bucket);
    if (index >= bucket_value_count) {
        assert(false && "error: cecs_flatbucket_swap_last_pop called with out of bounds index");
        exit(EXIT_FAILURE);
    }

    switch (bucket_value_count) {
    case 0: {
        assert(false && "error: cecs_flatbucket_swap_last_pop called with empty bucket");
        exit(EXIT_FAILURE);
    }
    case 1: {
        --bucket->hash_from_index8_u7;
        break;
    }
    default: {
        const uint_fast8_t last_index = bucket_value_count - 1;
        const void *last_value = cecs_flatbucket_get_value(bucket, last_index, value_size, values_offset);
        void *const value = cecs_flatbucket_get_value_mut(bucket, index, value_size, values_offset);
        memcpy(value, last_value, value_size);

        --bucket->hash_from_index8_u7;
        break;
    }
    }
}
void cecs_flatbucket_remove_expect(
    cecs_flatbucket *bucket,
    const uint_fast8_t index,
    const size_t value_size,
    const size_t values_offset
) {
    cecs_flatbucket_swap_last_pop(bucket, index, value_size, values_offset);
    const uint_fast8_t last_index = cecs_flatbucket_get_count(*bucket);
    const cecs_flatbucket_hash_low_fast last_hash = cecs_flatbucket_get_hash_low(*bucket, last_index);
    cecs_flatbucket_set_hash_low(bucket, index, last_hash);
}

static uint8_t cecs_flatbucket_mark_hash_low_index(
    const cecs_flatbucket bucket,
    const cecs_flatbucket_hash_low_fast hash7
) {
    if (hash7 >= cecs_flatbucket_hash7_max) {
        assert(false && "error: cecs_flatbucket_mark_hash_low_index called with out of bounds hash_low");
        exit(EXIT_FAILURE);
    }
    const uint8_t index_mark = cecs_mark_pattern_byteshp8_u7(bucket.hash_from_index8_u7, hash7);
    return index_mark;
}
uint_fast8_t cecs_flatbucket_find_hash_index(
    const cecs_flatbucket *bucket,
    const cecs_flatbucket_hash hash,
    const cecs_flatbucket_hash_low_fast hash7,
    const size_t hash_offset,
    const size_t hash_stride
) {
    const uint8_t index_mark = cecs_flatbucket_mark_hash_low_index(*bucket, hash7);
    const uint_fast8_t first_index = cecs_tzcnt_u8(index_mark);
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*bucket);
    if (first_index >= bucket_value_count) {
        return CECS_FLATBUCKET8_MAX_COUNT;
    }
    
    for (uint_fast8_t i = 0; i < bucket_value_count; i++) {
        const cecs_flatbucket_hash *stored_hash = (const cecs_flatbucket_hash *)(bucket->values + i * hash_stride + hash_offset);
        if (
            (index_mark & (1 << i))
            && (*stored_hash == hash)
        ) {
            return i;
        }
    }
    return CECS_FLATBUCKET8_MAX_COUNT;
}

void cecs_flatbucket_reset(cecs_flatbucket *bucket, const size_t value_size) {
    *bucket = (cecs_flatbucket){0};
    memset(bucket->values, 0, cecs_flatbucket_max_count * value_size);
}
