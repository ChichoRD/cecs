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

#define CECS_FLATBUCKET8_MAX_COUNT_LOG2 3
#define CECS_FLATBUCKET8_MAX_COUNT (1 << CECS_FLATBUCKET8_MAX_COUNT_LOG2)
extern const uint_fast8_t cecs_flatbucket8_max_count;

uint_fast8_t cecs_flatbucket_get_count(const cecs_flatbucket8 bucket);

typedef struct cecs_flatset {
    cecs_flatbucket8 *buckets;
    size_t bucket_count;
    size_t values_count;
} cecs_flatset;

static inline size_t cecs_flatset_count(const cecs_flatset *set) {
    return set->values_count;
}
static inline size_t cecs_flatset_bucket_count(const cecs_flatset *set) {
    return set->bucket_count;
}

static inline const cecs_flatbucket8 *cecs_flatset_get_bucket(const cecs_flatset *set, const size_t bucket_index, const size_t value_size) {
    if (bucket_index >= set->bucket_count) {
        assert(false && "error: cecs_flatset_get_bucket called with out of bounds bucket index");
        exit(EXIT_FAILURE);
    }
    return ((uint8_t *)&set->buckets[bucket_index]) + (value_size * bucket_index * CECS_FLATBUCKET8_MAX_COUNT);
}
static inline cecs_flatbucket8 *cecs_flatset_get_bucket_mut(cecs_flatset *set, const size_t bucket_index, const size_t value_size) {
    if (bucket_index >= set->bucket_count) {
        assert(false && "error: cecs_flatset_get_bucket_mut called with out of bounds bucket index");
        exit(EXIT_FAILURE);
    }
    return ((uint8_t *)&set->buckets[bucket_index]) + (value_size * bucket_index * CECS_FLATBUCKET8_MAX_COUNT);
}

cecs_flatset cecs_flatset_create(void);
cecs_flatset cecs_flatset_create_with_capacity(cecs_allocator *allocator, const size_t bucket_count_log2, const size_t value_size);


void *cecs_flatset_insert(cecs_flatset *set, cecs_allocator *allocator, const cecs_flatset_hash hash, const size_t value_size);

#endif