#ifndef CECS_FLATSET_H
#define CECS_FLATSET_H

#include "cecs_allocator.h"
#include <stdint.h>
#include <assert.h>

#ifndef CECS_FLATSET_HASH_TYPE
#define CECS_FLATSET_HASH_TYPE_DEFAULT size_t
#define CECS_FLATSET_HASH_TYPE CECS_FLATSET_HASH_TYPE_DEFAULT
#endif

typedef CECS_FLATSET_HASH_TYPE cecs_flatset_hash;
typedef uint8_t cecs_flatset_hash_low;
typedef uint_fast8_t cecs_flatset_hash_low_fast;
static_assert(
    sizeof(cecs_flatset_hash_low) <= sizeof(cecs_flatset_hash),
    "static error: cecs_flatset_hash_low must be able to hold cecs_flatset_hash"
);
static_assert(
    sizeof(cecs_flatset_hash_low) <= sizeof(cecs_flatset_hash_low),
    "static error: cecs_flatset_hash_low_fast must be able to hold cecs_flatset_hash_low"
);

typedef struct cecs_flatbucket8 {
    uint32_t hash_from_position8_b4;
    uint32_t index_from_position8_b4;
    uint8_t values[];
} cecs_flatbucket8;

// Type alias for cleaner API
typedef cecs_flatbucket8 cecs_flatbucket;

#define CECS_FLATBUCKET8_MAX_COUNT_LOG2 3
#define CECS_FLATBUCKET8_MAX_COUNT (1 << CECS_FLATBUCKET8_MAX_COUNT_LOG2)
extern const uint_fast8_t cecs_flatbucket8_max_count;

// Bucket capacity and count functions
uint_fast8_t cecs_flatbucket_get_count(const cecs_flatbucket bucket);
extern inline bool cecs_flatbucket_is_full(const cecs_flatbucket bucket) {
    return bucket.index_from_position8_b4 & 0x00008000;
}

// Bucket value access functions
inline const void *cecs_flatbucket_get_value_by_index(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size) {
    if (index >= cecs_flatbucket8_max_count) {
        assert(false && "error: cecs_flatbucket_get_value_by_index called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return &bucket->values[index * value_size];
}
inline void *cecs_flatbucket_get_value_by_index_mut(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size) {
    if (index >= cecs_flatbucket8_max_count) {
        assert(false && "error: cecs_flatbucket_get_value_by_index_mut called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return &bucket->values[index * value_size];
}

typedef struct cecs_flatset {
    cecs_flatbucket *buckets;
    size_t bucket_count;
    size_t values_count;
} cecs_flatset;

// Set capacity and count functions
static inline size_t cecs_flatset_capacity(const cecs_flatset *set) {
    return set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT;
}
static inline size_t cecs_flatset_count(const cecs_flatset *set) {
    return set->values_count;
}
static inline size_t cecs_flatset_bucket_count(const cecs_flatset *set) {
    return set->bucket_count;
}

static inline size_t cecs_flatset_bucket_size(const size_t value_size) {
    return sizeof(cecs_flatbucket) + (value_size * CECS_FLATBUCKET8_MAX_COUNT);
}

// Set bucket access functions
static inline const cecs_flatbucket *cecs_flatset_get_bucket(const cecs_flatset *set, const size_t bucket_index, const size_t value_size) {
    if (bucket_index >= set->bucket_count) {
        assert(false && "error: cecs_flatset_get_bucket called with out of bounds bucket index");
        exit(EXIT_FAILURE);
    }
    return ((uint8_t *)set->buckets) + cecs_flatset_bucket_size(value_size) * bucket_index;
}
static inline cecs_flatbucket *cecs_flatset_get_bucket_mut(cecs_flatset *set, const size_t bucket_index, const size_t value_size) {
    if (bucket_index >= set->bucket_count) {
        assert(false && "error: cecs_flatset_get_bucket_mut called with out of bounds bucket index");
        exit(EXIT_FAILURE);
    }
    return ((uint8_t *)set->buckets) + cecs_flatset_bucket_size(value_size) * bucket_index;
}

// Set creation and destruction functions
cecs_flatset cecs_flatset_create(void);
cecs_flatset cecs_flatset_create_with_capacity(cecs_allocator *allocator, const size_t bucket_count, const size_t value_size);
void cecs_flatset_clear(cecs_flatset *set, const size_t value_size);
void cecs_flatset_destroy(cecs_flatset *set, cecs_allocator *allocator, const size_t value_size);

// Set memory management functions
void cecs_flatset_copy(
    cecs_flatset *destination,
    const cecs_flatset *source,
    cecs_allocator *allocator,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
void cecs_flatset_resize(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
void cecs_flatset_shrink(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);

// Set search functions
bool cecs_flatset_find(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    const void **out_value
);
bool cecs_flatset_find_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    void **out_value
);
const void *cecs_flatset_find_expect(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
const void *cecs_flatset_find_expect_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);

// Set bucket search functions
bool cecs_flatset_find_bucket(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    const cecs_flatbucket **out_bucket,
    uint_fast8_t *out_position
);
bool cecs_flatset_find_bucket_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    cecs_flatbucket **out_bucket,
    uint_fast8_t *out_position
);
cecs_flatbucket *cecs_flatset_find_insert_bucket_expect(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
cecs_flatbucket *cecs_flatset_find_insert_bucket(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    uint_fast8_t *out_position
);

// Set insertion functions
void *cecs_flatset_insert_into_bucket_expect(
    cecs_flatset *set,
    cecs_flatbucket8 *bucket,
    const cecs_flatset_hash hash,
    const size_t value_size
);
void *cecs_flatset_insert_within_expect(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
void *cecs_flatset_insert_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
void *cecs_flatset_find_or_insert(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);

// Set removal functions
void cecs_flatset_remove_from_bucket_stable_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    cecs_flatbucket *bucket,
    const uint_fast8_t position,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
void cecs_flatset_remove_from_bucket_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    cecs_flatbucket *bucket,
    const uint_fast8_t position,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
bool cecs_flatset_find_remove(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);
void cecs_flatset_find_remove_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
);

#endif