#ifndef CECS_FLATSET_H
#define CECS_FLATSET_H


#include "cecs_flatbucket.h"
#include <cecs_allocator.h>
#include <stdint.h>
#include <assert.h>

typedef cecs_flatbucket_hash cecs_flatset_hash;
typedef cecs_flatbucket_hash_low cecs_flatset_hash_low;
typedef cecs_flatbucket_hash_low_fast cecs_flatset_hash_low_fast;

// Bucket value access functions
inline const void *cecs_flatset_bucket_get_value_unchecked(const cecs_flatbucket bucket, const uint_fast8_t index, const size_t value_size) {
    return cecs_flatbucket_get_value_unchecked(bucket, index, value_size);
}
inline void *cecs_flatset_bucket_get_value_mut_unchecked(const cecs_flatbucket_mut bucket, const uint_fast8_t index, const size_t value_size) {
    return cecs_flatbucket_get_value_mut_unchecked(bucket, index, value_size);
}
inline const void *cecs_flatset_bucket_get_value(const cecs_flatbucket bucket, const uint_fast8_t index, const size_t value_size) {
    return cecs_flatbucket_get_value(bucket, index, value_size);
}
inline void *cecs_flatset_bucket_get_value_mut(const cecs_flatbucket_mut bucket, const uint_fast8_t index, const size_t value_size) {
    return cecs_flatbucket_get_value_mut(bucket, index, value_size);
}

typedef struct cecs_flatset {
    // cecs_flatbucket *buckets;
    // {{([hash], value), ([hash],    value), ...}, [hash,    hash,   ...], {ctl,   ctl,    ...}}
    // {{([u64],  size),  ([u64],     size),  ...}, [u64,     u64,    ...], {u7u1,  u7u1,   ...}}
    unsigned char *allocation;
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
static inline size_t cecs_flatset_allocation_size(const size_t value_size, const size_t bucket_count) {
    return value_size * bucket_count * CECS_FLATBUCKET8_MAX_COUNT
        + sizeof(cecs_flatbucket) * bucket_count;
}

// Set bucket access functions
static inline cecs_flatbucket cecs_flatset_get_bucket(
    const cecs_flatset *set,
    const size_t bucket_index,
    const size_t value_size,
    const size_t hash_offset
) {
    if (bucket_index >= set->bucket_count) {
        assert(false && "error: cecs_flatset_get_bucket called with out of bounds bucket index");
        exit(EXIT_FAILURE);
    }

    return (cecs_flatbucket){
        .hash_from_index8_u7 = (uint64_t *)(set->allocation
            + value_size + set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT
            + sizeof(cecs_flatbucket) * bucket_index),
        .hashes = (cecs_flatbucket_hash *)(set->allocation
            + value_size * bucket_index * CECS_FLATBUCKET8_MAX_COUNT
            + hash_offset),
        .values = set->allocation
            + value_size * bucket_index * CECS_FLATBUCKET8_MAX_COUNT
    };
}
static inline cecs_flatbucket_mut cecs_flatset_get_bucket_mut(
    cecs_flatset *set,
    const size_t bucket_index,
    const size_t value_size,
    const size_t hash_offset
) {
    if (bucket_index >= set->bucket_count) {
        assert(false && "error: cecs_flatset_get_bucket_mut called with out of bounds bucket index");
        exit(EXIT_FAILURE);
    }
    return (cecs_flatbucket_mut) {
        .hash_from_index8_u7 = (uint64_t *)(set->allocation
            + value_size + set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT
            + sizeof(cecs_flatbucket) * bucket_index),
        .hashes = (cecs_flatbucket_hash *)(set->allocation
            + value_size * bucket_index * CECS_FLATBUCKET8_MAX_COUNT
            + hash_offset),
        .values = set->allocation
            + value_size * bucket_index * CECS_FLATBUCKET8_MAX_COUNT
    };
}

// Set creation and destruction functions
cecs_flatset cecs_flatset_create(void);
cecs_flatset cecs_flatset_create_with_capacity(cecs_allocator *allocator, const size_t bucket_count, const size_t value_size);
void cecs_flatset_clear(cecs_flatset *set, const size_t value_size);
void cecs_flatset_destroy(cecs_flatset *set, cecs_allocator *allocator, const size_t value_size);

// Set memory management functions
void cecs_flatset_extend_exclusive(
    cecs_flatset *destination,
    const cecs_flatset *source,
    const size_t value_size,
    const size_t hash_offset
);
void cecs_flatset_extend(
    cecs_flatset *destination,
    const cecs_flatset *source,
    cecs_allocator *allocator,
    const size_t value_size,
    const size_t hash_offset
);
void cecs_flatset_resize(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size,
    const size_t hash_offset
);
void cecs_flatset_shrink(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size,
    const size_t hash_offset
);

// Set search functions
bool cecs_flatset_find(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const void **out_value
);
bool cecs_flatset_find_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    void **out_value
);
const void *cecs_flatset_find_expect(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
);
const void *cecs_flatset_find_expect_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
);

// Set bucket search functions
bool cecs_flatset_find_bucket(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    cecs_flatbucket *out_bucket,
    uint_fast8_t *out_index
);
bool cecs_flatset_find_bucket_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    cecs_flatbucket_mut *out_bucket,
    uint_fast8_t *out_index
);
cecs_flatbucket_mut cecs_flatset_find_insert_bucket_expect(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
);
cecs_flatbucket_mut cecs_flatset_find_insert_bucket(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    uint_fast8_t *out_index
);

// Set insertion functions
void *cecs_flatset_insert_into_bucket_expect(
    cecs_flatset *set,
    const cecs_flatbucket_mut bucket,
    const cecs_flatset_hash hash,
    const size_t value_size
);
void *cecs_flatset_insert_within_expect(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
);
void *cecs_flatset_insert_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
);
void *cecs_flatset_find_or_insert(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
);

// Set removal functions
void cecs_flatset_remove_from_bucket_stable_expect(
    cecs_flatset *set,
    const cecs_flatbucket_mut bucket,
    const uint_fast8_t index,
    const size_t value_size
);
void cecs_flatset_remove_from_bucket_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatbucket_mut bucket,
    const uint_fast8_t index,
    const size_t value_size,
    const size_t hash_offset
);
bool cecs_flatset_find_remove(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
);
void cecs_flatset_find_remove_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
);

#endif
