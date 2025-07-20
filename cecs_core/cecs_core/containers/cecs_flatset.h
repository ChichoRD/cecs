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

typedef struct cecs_flatbucket16_count_psl {
    uint8_t count;
    uint8_t psl0_4;         // 2 bits each
    uint8_t psl4_8[2];      // 3 bits each --> 4 bits each
    uint8_t psl8_12[2];     // 4 bits each
    uint8_t psl12_16[2];    // 4 bits each
} cecs_flatbucket16_count_psl;
static_assert(
    sizeof(cecs_flatbucket16_count_psl) == sizeof(uint8_t[8]),
    "static error: cecs_flatbucket16_count_psl must be 8 bytes"
);

typedef struct cecs_flatbucket16_index_pair {
    uint8_t index0 : 4;
    uint8_t index1 : 4;
} cecs_flatbucket16_index_pair;
typedef union cecs_flatbucket16_index_pair_u {
    cecs_flatbucket16_index_pair pair;
    uint8_t value;
} cecs_flatbucket16_index_pair_u;

typedef struct cecs_flatbucket16 {
    cecs_flatbucket16_count_psl count_psl;
    cecs_flatbucket16_index_pair_u hash_to_indices[8];
    uint8_t values[];
} cecs_flatbucket16;


typedef struct cecs_flatset {
    cecs_flatbucket16 *buckets;
    size_t bucket_count;
    size_t values_count;
} cecs_flatset;

static inline size_t cecs_flatset_count(const cecs_flatset *set) {
    return set->values_count;
}
static inline size_t cecs_flatset_bucket_count(const cecs_flatset *set) {
    return set->bucket_count;
}

cecs_flatset cecs_flatset_create(void);
cecs_flatset cecs_flatset_create_with_capacity(cecs_allocator *allocator, const size_t bucket_count_log2, const size_t value_size);


void *cecs_flatset_insert(cecs_flatset *set, cecs_allocator *allocator, const cecs_flatset_hash hash, const size_t value_size);

#endif