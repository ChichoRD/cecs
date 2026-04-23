#ifndef CECS_FLATBUCKET_H
#define CECS_FLATBUCKET_H

#include <cecs_error.h>
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
    const uint64_t *hash_from_index8_u7;
    const cecs_flatbucket_hash *hashes;
    const unsigned char *values;
} cecs_flatbucket8;
typedef struct cecs_flatbucket8_mut {
    uint64_t *hash_from_index8_u7;
    cecs_flatbucket_hash *hashes;
    unsigned char *values;
} cecs_flatbucket8_mut;

// Type alias for cleaner API
typedef cecs_flatbucket8 cecs_flatbucket;
typedef cecs_flatbucket8_mut cecs_flatbucket_mut;
static_assert(
    sizeof(cecs_flatbucket) == sizeof(cecs_flatbucket_mut),
    "static error: cecs_flatbucket and cecs_flatbucket_mut must be the same size"
);
static_assert(
    offsetof(cecs_flatbucket, hash_from_index8_u7) == offsetof(cecs_flatbucket_mut, hash_from_index8_u7)
    && offsetof(cecs_flatbucket, hashes) == offsetof(cecs_flatbucket_mut, hashes)
    && offsetof(cecs_flatbucket, values) == offsetof(cecs_flatbucket_mut, values),
    "static error: cecs_flatbucket and cecs_flatbucket_mut must have the same layout"
);

#define CECS_FLATBUCKET8_MAX_COUNT_LOG2 3ull
#define CECS_FLATBUCKET8_MAX_COUNT (1ull << CECS_FLATBUCKET8_MAX_COUNT_LOG2)
#define CECS_FLATBUCKET8_MAX_COUNT_MASK ((CECS_FLATBUCKET8_MAX_COUNT << 1ull) - 1ull)

extern const uint_fast8_t cecs_flatbucket_max_count;
cecs_flatbucket_hash_low_fast cecs_flatbucket_hash_low_get(const cecs_flatbucket_hash hash);

inline cecs_flatbucket cecs_flatbucket_from(const cecs_flatbucket_mut bucket) {
    return (cecs_flatbucket){
        .hash_from_index8_u7 = bucket.hash_from_index8_u7,
        .hashes = bucket.hashes,
        .values = bucket.values,
        // .error2 = bucket.error2
    };
}

// Bucket capacity and count functions
inline uint_fast8_t cecs_flatbucket_get_count(const cecs_flatbucket bucket) {
    static_assert(
        CECS_FLATBUCKET8_MAX_COUNT_MASK <= UINT_FAST8_MAX,
        "static error: value masked with CECS_FLATBUCKET8_MAX_COUNT_MASK must fit within uint_fast8_t"
    );
    static const uint64_t count_mask = CECS_FLATBUCKET8_MAX_COUNT_MASK;
    return (uint_fast8_t)((*bucket.hash_from_index8_u7) & count_mask);
}
inline bool cecs_flatbucket_is_full(const cecs_flatbucket bucket) {
    static const uint64_t full_mask = (1ull << CECS_FLATBUCKET8_MAX_COUNT_LOG2);
    return (*bucket.hash_from_index8_u7) & full_mask;
}
inline bool cecs_flatbucket_has_been_full(const cecs_flatbucket bucket) {
    static const uint64_t been_full_mask = 0x80ull;
    return (*bucket.hash_from_index8_u7) & been_full_mask;
}

// Bucket value access functions
inline const void *cecs_flatbucket_get_value_unchecked(const cecs_flatbucket bucket, const uint_fast8_t index, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        index < CECS_FLATBUCKET8_MAX_COUNT,
        "error: cecs_flatbucket_get_value_unchecked called with out of bounds index"
    );
    return &bucket.values[index * value_size];
}
inline void *cecs_flatbucket_get_value_mut_unchecked(const cecs_flatbucket_mut bucket, const uint_fast8_t index, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        index < CECS_FLATBUCKET8_MAX_COUNT,
        "error: cecs_flatbucket_get_value_mut_unchecked called with out of bounds index"
    );
    return &bucket.values[index * value_size];
}
inline const void *cecs_flatbucket_get_value(const cecs_flatbucket bucket, const uint_fast8_t index, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        index < cecs_flatbucket_get_count(bucket),
        "error: cecs_flatbucket_get_value called with out of bounds index"
    );
    return &bucket.values[index * value_size];
}
inline void *cecs_flatbucket_get_value_mut(const cecs_flatbucket_mut bucket, const uint_fast8_t index, const size_t value_size) {
    cecs_debugbreak_fail_unless(
        index < cecs_flatbucket_get_count(cecs_flatbucket_from(bucket)),
        "error: cecs_flatbucket_get_value_mut called with out of bounds index"
    );
    return &bucket.values[index * value_size];
}

inline const cecs_flatbucket_hash *cecs_flatbucket_get_hash_unchecked(const cecs_flatbucket bucket, const uint_fast8_t index, const size_t stride) {
    cecs_debugbreak_fail_unless(
        index < CECS_FLATBUCKET8_MAX_COUNT,
        "error: cecs_flatbucket_get_hash_unchecked called with out of bounds index"
    );
    return (const cecs_flatbucket_hash *)(((const unsigned char *)bucket.hashes) + index * stride);
}
inline cecs_flatbucket_hash *cecs_flatbucket_get_hash_mut_unchecked(const cecs_flatbucket_mut bucket, const uint_fast8_t index, const size_t stride) {
    cecs_debugbreak_fail_unless(
        index < CECS_FLATBUCKET8_MAX_COUNT,
        "error: cecs_flatbucket_get_hash_mut_unchecked called with out of bounds index"
    );
    return (cecs_flatbucket_hash *)(((unsigned char *)bucket.hashes) + index * stride);
}
inline const cecs_flatbucket_hash *cecs_flatbucket_get_hash(const cecs_flatbucket bucket, const uint_fast8_t index, const size_t stride) {
    cecs_debugbreak_fail_unless(
        index < cecs_flatbucket_get_count(bucket),
        "error: cecs_flatbucket_get_hash called with out of bounds index"
    );
    return (const cecs_flatbucket_hash *)(((const unsigned char *)bucket.hashes) + index * stride);
}
inline cecs_flatbucket_hash *cecs_flatbucket_get_hash_mut(const cecs_flatbucket_mut bucket, const uint_fast8_t index, const size_t stride) {
    cecs_debugbreak_fail_unless(
        index < cecs_flatbucket_get_count(cecs_flatbucket_from(bucket)),
        "error: cecs_flatbucket_get_hash_mut called with out of bounds index"
    );
    return (cecs_flatbucket_hash *)(((unsigned char *)bucket.hashes) + index * stride);
}


void *cecs_flatbucket_insert_expect(
    const cecs_flatbucket_mut bucket,
    const uint_fast8_t index,
    const cecs_flatbucket_hash_low hash7,
    const size_t value_size
);
void cecs_flatbucket_remove_expect(
    const cecs_flatbucket_mut bucket,
    const uint_fast8_t index,
    const size_t value_size
);

uint_fast8_t cecs_flatbucket_find_hash_index(
    const cecs_flatbucket bucket,
    const cecs_flatbucket_hash hash,
    const cecs_flatbucket_hash_low_fast hash7,
    const size_t hash_stride
);
void cecs_flatbucket_reset(const cecs_flatbucket_mut bucket, const size_t value_size);

#endif
