#ifndef CECS_FLATMAP_H
#define CECS_FLATMAP_H

#include "cecs_flatbucket.h"

#include <cecs_allocator.h>
#include <stdint.h>
#include <assert.h>

typedef cecs_flatbucket_hash cecs_flatmap_hash;
typedef cecs_flatbucket_hash_low cecs_flatmap_hash_low;
typedef cecs_flatbucket_hash_low_fast cecs_flatmap_hash_low_fast;

// Bucket value access functions
inline const void *cecs_flatmap_bucket_get_value_unchecked(const cecs_flatbucket bucket, const uint_fast8_t index, const size_t value_size) {
    return cecs_flatbucket_get_value_unchecked(bucket, index, value_size);
}
inline void *cecs_flatmap_bucket_get_value_mut_unchecked(const cecs_flatbucket_mut bucket, const uint_fast8_t index, const size_t value_size) {
    return cecs_flatbucket_get_value_mut_unchecked(bucket, index, value_size);
}
inline const void *cecs_flatmap_bucket_get_value(const cecs_flatbucket bucket, const uint_fast8_t index, const size_t value_size) {
    return cecs_flatbucket_get_value(bucket, index, value_size);
}
inline void *cecs_flatmap_bucket_get_value_mut(const cecs_flatbucket_mut bucket, const uint_fast8_t index, const size_t value_size) {
    return cecs_flatbucket_get_value_mut(bucket, index, value_size);
}

inline const cecs_flatmap_hash *cecs_flatmap_bucket_get_key_unchecked(const cecs_flatbucket bucket, const uint_fast8_t index) {
    return (const cecs_flatmap_hash *)cecs_flatbucket_get_hash_unchecked(bucket, index, sizeof(cecs_flatmap_hash));
}
inline cecs_flatmap_hash *cecs_flatmap_bucket_get_key_mut_unchecked(const cecs_flatbucket_mut bucket, const uint_fast8_t index) {
    return (cecs_flatmap_hash *)cecs_flatbucket_get_hash_mut_unchecked(bucket, index, sizeof(cecs_flatmap_hash));
}
inline const cecs_flatmap_hash *cecs_flatmap_bucket_get_key(const cecs_flatbucket bucket, const uint_fast8_t index) {
    return (const cecs_flatmap_hash *)cecs_flatbucket_get_hash(bucket, index, sizeof(cecs_flatmap_hash));
}
inline cecs_flatmap_hash *cecs_flatmap_bucket_get_key_mut(const cecs_flatbucket_mut bucket, const uint_fast8_t index) {
    return (cecs_flatmap_hash *)cecs_flatbucket_get_hash_mut(bucket, index, sizeof(cecs_flatmap_hash));
}


typedef struct cecs_flatmap {
    // {{([hash], value), ([hash],    value), ...}, [hash,    hash,   ...], {ctl,   ctl,    ...}}
    // {{([u64],  size),  ([u64],     size),  ...}, [u64,     u64,    ...], {u7u1,  u7u1,   ...}}
    unsigned char *allocation;
    size_t bucket_count;
    size_t values_count;
} cecs_flatmap;

// Set capacity and count functions
static inline size_t cecs_flatmap_capacity(const cecs_flatmap *set) {
    return set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT;
}
static inline size_t cecs_flatmap_count(const cecs_flatmap *set) {
    return set->values_count;
}
static inline size_t cecs_flatmap_bucket_count(const cecs_flatmap *set) {
    return set->bucket_count;
}
static inline size_t cecs_flatmap_allocation_size(const size_t value_size, const size_t bucket_count) {
    return value_size * bucket_count * CECS_FLATBUCKET8_MAX_COUNT
        + sizeof(cecs_flatmap_hash) * bucket_count * CECS_FLATBUCKET8_MAX_COUNT
        + sizeof(cecs_flatbucket) * bucket_count;
}

// Set bucket access functions
static inline cecs_flatbucket cecs_flatmap_get_bucket(const cecs_flatmap *set, const size_t bucket_index, const size_t value_size) {
    if (bucket_index >= set->bucket_count) {
        assert(false && "error: cecs_flatmap_get_bucket called with out of bounds bucket index");
        exit(EXIT_FAILURE);
    }
    return (cecs_flatbucket){
        .hash_from_index8_u7 = (uint64_t *)(set->allocation
            + value_size * set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT
            + sizeof(cecs_flatmap_hash) * set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT
            + sizeof(cecs_flatbucket) * bucket_index),
        .hashes = (cecs_flatmap_hash *)(set->allocation
            + value_size * set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT
            + sizeof(cecs_flatmap_hash) * bucket_index * CECS_FLATBUCKET8_MAX_COUNT),
        .values = set->allocation
            + value_size * bucket_index * CECS_FLATBUCKET8_MAX_COUNT
    };
}
static inline cecs_flatbucket_mut cecs_flatmap_get_bucket_mut(cecs_flatmap *set, const size_t bucket_index, const size_t value_size) {
    if (bucket_index >= set->bucket_count) {
        assert(false && "error: cecs_flatmap_get_bucket_mut called with out of bounds bucket index");
        exit(EXIT_FAILURE);
    }
    return (cecs_flatbucket_mut){
        .hash_from_index8_u7 = (uint64_t *)(set->allocation
            + value_size * set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT
            + sizeof(cecs_flatmap_hash) * set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT
            + sizeof(cecs_flatbucket) * bucket_index),
        .hashes = (cecs_flatmap_hash *)(set->allocation
            + value_size * set->bucket_count * CECS_FLATBUCKET8_MAX_COUNT
            + sizeof(cecs_flatmap_hash) * bucket_index * CECS_FLATBUCKET8_MAX_COUNT),
        .values = set->allocation
            + value_size * bucket_index * CECS_FLATBUCKET8_MAX_COUNT
    };
}

// Set creation and destruction functions
cecs_flatmap cecs_flatmap_create(void);
cecs_flatmap cecs_flatmap_create_with_capacity(cecs_allocator *allocator, const size_t bucket_count, const size_t value_size);
void cecs_flatmap_clear(cecs_flatmap *set, const size_t value_size);
void cecs_flatmap_destroy(cecs_flatmap *set, cecs_allocator *allocator, const size_t value_size);

// Set memory management functions
void cecs_flatmap_extend_exclusive(
    cecs_flatmap *destination,
    const cecs_flatmap *source,
    const size_t value_size
);
void cecs_flatmap_extend(
    cecs_flatmap *destination,
    const cecs_flatmap *source,
    cecs_allocator *allocator,
    const size_t value_size
);
void cecs_flatmap_resize(
    cecs_flatmap *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size
);
void cecs_flatmap_shrink(
    cecs_flatmap *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size
);

// Set search functions
bool cecs_flatmap_find(
    const cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size,
    const void **out_value
);
bool cecs_flatmap_find_mut(
    cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size
    ,
    void **out_value
);
const void *cecs_flatmap_find_expect(
    const cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size
);
const void *cecs_flatmap_find_expect_mut(
    cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size
);

// Set bucket search functions
bool cecs_flatmap_find_bucket(
    const cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size,
    cecs_flatbucket *out_bucket,
    uint_fast8_t *out_index
);
bool cecs_flatmap_find_bucket_mut(
    cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size,
    cecs_flatbucket_mut *out_bucket,
    uint_fast8_t *out_index
);
cecs_flatbucket_mut cecs_flatmap_find_insert_bucket_expect(
    cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size
    
);
cecs_flatbucket_mut cecs_flatmap_find_insert_bucket(
    cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size,
    uint_fast8_t *out_index
);

// Set insertion functions
void *cecs_flatmap_insert_into_bucket_expect(
    cecs_flatmap *set,
    const cecs_flatbucket_mut bucket,
    const cecs_flatmap_hash hash,
    const size_t value_size
);
void *cecs_flatmap_insert_within_expect(
    cecs_flatmap *set,
    const cecs_flatmap_hash hash,
    const size_t value_size
);
void *cecs_flatmap_insert_expect(
    cecs_flatmap *set,
    cecs_allocator *allocator,
    const cecs_flatmap_hash hash,
    const size_t value_size
);
void *cecs_flatmap_find_or_insert(
    cecs_flatmap *set,
    cecs_allocator *allocator,
    const cecs_flatmap_hash hash,
    const size_t value_size
);

// Set removal functions
void cecs_flatmap_remove_from_bucket_stable_expect(
    cecs_flatmap *set,
    const cecs_flatbucket_mut bucket,
    const uint_fast8_t index,
    const size_t value_size
);
void cecs_flatmap_remove_from_bucket_expect(
    cecs_flatmap *set,
    cecs_allocator *allocator,
    const cecs_flatbucket_mut bucket,
    const uint_fast8_t index,
    const size_t value_size
);
bool cecs_flatmap_find_remove(
    cecs_flatmap *set,
    cecs_allocator *allocator,
    const cecs_flatmap_hash hash,
    const size_t value_size
);
void cecs_flatmap_find_remove_expect(
    cecs_flatmap *set,
    cecs_allocator *allocator,
    const cecs_flatmap_hash hash,
    const size_t value_size
);

#endif
