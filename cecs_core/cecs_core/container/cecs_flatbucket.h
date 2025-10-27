#ifndef CECS_FLATBUCKET_H
#define CECS_FLATBUCKET_H

#include <cecs_core/cecs_error.h>
#include <stdint.h>
#include <assert.h>

#ifndef CECS_FLATBUCKET_HASH_TYPE
#define CECS_FLATBUCKET_HASH_TYPE_DEFAULT size_t
#define CECS_FLATBUCKET_HASH_TYPE CECS_FLATBUCKET_HASH_TYPE_DEFAULT
#endif

typedef CECS_FLATBUCKET_HASH_TYPE cecs_flatbucket_hash;
typedef uint8_t cecs_flatbucket_hash_low;
typedef uint_fast8_t cecs_flatbucket_hash_low_fast;
static_assert(
    sizeof(cecs_flatbucket_hash_low) <= sizeof(cecs_flatbucket_hash),
    "static error: cecs_flatbucket_hash_low must be able to hold cecs_flatbucket_hash"
);
static_assert(
    sizeof(cecs_flatbucket_hash_low) <= sizeof(cecs_flatbucket_hash_low),
    "static error: cecs_flatbucket_hash_low_fast must be able to hold cecs_flatbucket_hash_low"
);

typedef struct cecs_flatbucket8 {
    uint64_t hash_from_index8_u7;
    uint8_t values[];
} cecs_flatbucket8;

// Type alias for cleaner API
typedef cecs_flatbucket8 cecs_flatbucket;
#define CECS_FLATBUCKET8_MAX_COUNT_LOG2 3
#define CECS_FLATBUCKET8_MAX_COUNT (1 << CECS_FLATBUCKET8_MAX_COUNT_LOG2)

extern const uint_fast8_t cecs_flatbucket_max_count;
cecs_flatbucket_hash_low_fast cecs_flatbucket_hash_low_get(const cecs_flatbucket_hash hash);

// Bucket capacity and count functions
inline uint_fast8_t cecs_flatbucket_get_count(const cecs_flatbucket bucket) {
    return bucket.hash_from_index8_u7 & 0x0F;
}
inline bool cecs_flatbucket_is_full(const cecs_flatbucket bucket) {
    return bucket.hash_from_index8_u7 & 0x08;
}
inline bool cecs_flatbucket_has_been_full(const cecs_flatbucket bucket) {
    return bucket.hash_from_index8_u7 & 0x80ull;
}

// Bucket value access functions
inline const void *cecs_flatbucket_get_value_unchecked(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t offset) {
    cecs_assert_or_exit(
        index < CECS_FLATBUCKET8_MAX_COUNT,
        "error: cecs_flatbucket_get_value_unchecked called with out of bounds index"
    );
    return &bucket->values[index * value_size + offset];
}
inline void *cecs_flatbucket_get_value_mut_unchecked(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t offset) {
    cecs_assert_or_exit(
        index < CECS_FLATBUCKET8_MAX_COUNT,
        "error: cecs_flatbucket_get_value_mut_unchecked called with out of bounds index"
    );
    return &bucket->values[index * value_size + offset];
}
inline const void *cecs_flatbucket_get_value(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t offset) {
    cecs_assert_or_exit(
        index < cecs_flatbucket_get_count(*bucket),
        "error: cecs_flatbucket_get_value called with out of bounds index"
    );
    return &bucket->values[index * value_size + offset];
}
inline void *cecs_flatbucket_get_value_mut(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size, const size_t offset) {
    cecs_assert_or_exit(
        index < cecs_flatbucket_get_count(*bucket),
        "error: cecs_flatbucket_get_value_mut called with out of bounds index"
    );
    return &bucket->values[index * value_size + offset];
}


void *cecs_flatbucket_insert_expect(
    cecs_flatbucket *bucket,
    const uint_fast8_t index,
    const cecs_flatbucket_hash_low hash7,
    const size_t value_size,
    const size_t values_offset
);
void cecs_flatbucket_remove_expect(
    cecs_flatbucket *bucket,
    const uint_fast8_t index,
    const size_t value_size,
    const size_t values_offset
);

uint_fast8_t cecs_flatbucket_find_hash_index(
    const cecs_flatbucket *bucket,
    const cecs_flatbucket_hash hash,
    const cecs_flatbucket_hash_low_fast hash7,
    const size_t hash_offset,
    const size_t hash_stride
);
void cecs_flatbucket_reset(cecs_flatbucket *bucket, const size_t value_size);

#endif
