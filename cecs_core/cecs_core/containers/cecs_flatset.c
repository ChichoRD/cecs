#include "cecs_flatset.h"
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>
#include <cecs_math/relations/cecs_ordering.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <memory.h>

const uint_fast8_t cecs_flatbucket8_max_count = CECS_FLATBUCKET8_MAX_COUNT;

#define CECS_FLATBUCKET8_POSITION_MAX CECS_FLATBUCKET8_MAX_COUNT
#define CECS_FLATBUCKET8_POSITION_MASK (CECS_FLATBUCKET8_POSITION_MAX - 1)
#define CECS_FLATBUCKET8_HASH4_MAX_LOG2 4
#define CECS_FLATBUCKET8_HASH4_MAX (1 << CECS_FLATBUCKET8_HASH4_MAX_LOG2)
#define CECS_FLATBUCKET8_HASH4_MASK (CECS_FLATBUCKET8_HASH4_MAX - 1)

static const cecs_flatset_hash_low_fast cecs_flatbucket8_position_max = CECS_FLATBUCKET8_POSITION_MAX;
static const cecs_flatset_hash_low_fast cecs_flatbucket8_hash4_max = CECS_FLATBUCKET8_HASH4_MAX;

static inline uint_fast8_t cecs_flatbucket8_get_index(const cecs_flatbucket8 bucket, const uint_fast8_t position) {
    if (position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_get_index called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    return (bucket.index_from_position8_b4 >> (position << 2)) & 0b0111;
}
static inline void cecs_flatbucket8_set_index(cecs_flatbucket8 *bucket, const uint_fast8_t position, const uint_fast8_t index) {
    if (position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_set_index called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    if (index >= cecs_flatbucket8_max_count) {
        assert(false && "error: cecs_flatbucket8_set_index called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    bucket->index_from_position8_b4 &= ~(0b0111 << (position << 2));
    bucket->index_from_position8_b4 |= (index & 0b0111) << (position << 2);
}

static inline uint_fast8_t cecs_flatbucket8_find_position(const cecs_flatbucket8 bucket, const uint_fast8_t index) {
    if (index >= cecs_flatbucket8_max_count) {
        assert(false && "error: cecs_flatbucket8_get_position called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    const uint8_t index_mark = cecs_mark_pattern_nibbles8_u4(bucket.index_from_position8_b4 & 0x77777777, index);
    if (!cecs_is_pow2_u8(index_mark)) {
        assert(false && "error: cecs_flatbucket8_get_position detected that two positions exist that map to the same index");
        exit(EXIT_FAILURE);
    }
    const uint_fast8_t position = CHAR_BIT - cecs_lzcnt_u8(index_mark);
    return position;
}

static inline cecs_flatset_hash_low_fast cecs_flatbucket8_get_hash_low(const cecs_flatbucket8 bucket, const uint_fast8_t position) {
    if (position >= cecs_flatbucket8_max_count) {
        assert(false && "error: cecs_flatbucket8_get_hash_low called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    return (bucket.hash_from_position8_b4 >> (position << 2)) & 0b11111;
}
static inline void cecs_flatbucket8_set_hash_low(cecs_flatbucket8 *bucket, const uint_fast8_t position, const cecs_flatset_hash_low_fast hash4) {
    if (position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_set_hash_low called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    if (hash4 >= cecs_flatbucket8_hash4_max) {
        assert(false && "error: cecs_flatbucket8_set_hash_low called with out of bounds hash_low4");
        exit(EXIT_FAILURE);
    }
    bucket->hash_from_position8_b4 &= ~(0b11111 << (position << 2));
    bucket->hash_from_position8_b4 |= (hash4 & 0b11111) << (position << 2);
}

typedef struct cecs_flatbucket8_count_chain_length_full {
    uint_fast8_t count_chain_length_full;
} cecs_flatbucket8_count_chain_length_full;
static inline cecs_flatbucket8_count_chain_length_full cecs_flatbucket8_count_chain_length_create(
    const uint_fast8_t count,
    const uint_fast8_t chain_length,
    const uint_fast8_t full
) {
    if (count >= 16 || chain_length >= 8 || full >= 2) {
        assert(false && "error: cecs_flatbucket8_count_chain_length_create called with out of bounds count, chain_length or full");
        exit(EXIT_FAILURE);
    }
    return (cecs_flatbucket8_count_chain_length_full) {
        .count_chain_length_full = 
            (full << 7)
            | ((chain_length & 0b00000111) << 4)
            | (count & 0b00001111)
    };
}
static inline uint_fast8_t cecs_flatbucket8_count_chain_length_get_count(const cecs_flatbucket8_count_chain_length_full count_chain_length) {
    return count_chain_length.count_chain_length_full & 0b00001111;
}
static inline uint_fast8_t cecs_flatbucket8_count_chain_length_get_length(const cecs_flatbucket8_count_chain_length_full count_chain_length) {
    return (count_chain_length.count_chain_length_full >> 4) & 0b00001111;
}
static inline bool cecs_flatbucket8_count_chain_length_has_been_full(const cecs_flatbucket8_count_chain_length_full count_chain_length) {
    return count_chain_length.count_chain_length_full & 0b10000000;
}

static inline cecs_flatbucket8_count_chain_length_full cecs_flatbucket8_get_count_chain_length_full(const cecs_flatbucket8 bucket) {
    return (cecs_flatbucket8_count_chain_length_full) {
        .count_chain_length_full = cecs_gather_msn8_u4(bucket.index_from_position8_b4)
    };
}
static inline void cecs_flatbucket8_set_count(cecs_flatbucket8 *bucket, const uint_fast8_t count) {
    if (count >= 16) {
        assert(false && "error: cecs_flatbucket8_set_count called with out of bounds count");
        exit(EXIT_FAILURE);
    }
    bucket->index_from_position8_b4 &= 0xFFFF7777;
    bucket->index_from_position8_b4 |= cecs_scatter_msn8_u1(count);
}
static inline void cecs_flatbucket8_set_chain_length(cecs_flatbucket8 *bucket, const uint_fast8_t chain_length) {
    if (chain_length >= 8) {
        assert(false && "error: cecs_flatbucket8_set_chain_length called with out of bounds chain_length");
        exit(EXIT_FAILURE);
    }
    bucket->index_from_position8_b4 &= 0xF777FFFF;
    bucket->index_from_position8_b4 |= cecs_scatter_msn8_u1(chain_length << 4);
}
static inline void cecs_flatbucket8_set_been_full(cecs_flatbucket8 *bucket) {
    bucket->index_from_position8_b4 |= 0x80000000;
}
static inline void cecs_flatbucket8_unset_been_full(cecs_flatbucket8 *bucket) {
    bucket->index_from_position8_b4 &= ~0x80000000;
}

uint_fast8_t cecs_flatbucket_get_count(const cecs_flatbucket bucket) {
    const cecs_flatbucket8_count_chain_length_full count_chain_length = cecs_flatbucket8_get_count_chain_length_full(bucket);
    return cecs_flatbucket8_count_chain_length_get_count(count_chain_length);
}
extern inline bool cecs_flatbucket_is_full(const cecs_flatbucket bucket);

static inline const void *cecs_flatbucket_get_value_by_index(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size) {
    return (const void *)&bucket->values[index * value_size];
}
static inline void *cecs_flatbucket_get_value_by_index_mut(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size) {
    return (void *)&bucket->values[index * value_size];
}

static const void *cecs_flatbucket8_get_value(const cecs_flatbucket8 *bucket, const uint_fast8_t position, const uint_fast8_t bucket_value_count, const size_t value_size) {
    if (position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_get_value called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    const uint_fast8_t index = cecs_flatbucket8_get_index(*bucket, position);
    return cecs_flatbucket8_get_value_by_index(bucket, index, value_size);
}
static void *cecs_flatbucket8_get_value_mut(cecs_flatbucket8 *bucket, const uint_fast8_t position, const uint_fast8_t bucket_value_count, const size_t value_size) {
    if (position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_get_value_mut called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    const uint_fast8_t index = cecs_flatbucket8_get_index(*bucket, position);
    return cecs_flatbucket8_get_value_by_index_mut(bucket, index, value_size);
}

static inline void *cecs_flatbucket8_push(cecs_flatbucket8 *bucket, const uint_fast8_t bucket_value_count, const size_t value_size) {
    if (bucket_value_count >= cecs_flatbucket8_max_count) {
        assert(false && "error: cecs_flatbucket8_push called with full bucket");
        exit(EXIT_FAILURE);
    }
    void *const value = &bucket->values[bucket_value_count * value_size];
    cecs_flatbucket8_set_count(bucket, bucket_value_count + 1);
    return value;
}
static void *cecs_flatbucket_insert_expect(
    cecs_flatbucket *bucket,
    const uint_fast8_t position,
    const cecs_flatset_hash_low hash4,
    const size_t value_size
) {
    const uint_fast8_t index = cecs_flatbucket8_get_index(*bucket, position);
    if (cecs_flatbucket8_is_full(*bucket)) {
        assert(false && "error: cecs_flatbucket_insert_expect called with full bucket");
        exit(EXIT_FAILURE);
    }
    
    const uint_fast8_t bucket_value_count = cecs_flatbucket8_get_count(*bucket);
    if (index < bucket_value_count) {
        assert(false && "error: cecs_flatbucket_insert_expect called with already occupied hash_low");
        exit(EXIT_FAILURE);
    } else {
        cecs_flatbucket8_set_index(bucket, position, bucket_value_count);
        cecs_flatbucket8_set_hash_low(bucket, position, hash4);
        void *const value = cecs_flatbucket8_push(bucket, bucket_value_count, value_size);
        if (cecs_flatbucket8_is_full(*bucket)) {
            cecs_flatbucket8_set_been_full(bucket);
        }
        return value;
    }
}

static inline void cecs_flatbucket8_swap_last_pop(cecs_flatbucket8 *bucket, const uint_fast8_t position, const uint_fast8_t bucket_value_count, const size_t value_size) {
    if (position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_swap_last_pop called with out of bounds position");
        exit(EXIT_FAILURE);
    }

    switch (bucket_value_count) {
    case 0: {
        assert(false && "error: cecs_flatbucket8_swap_last_pop called with empty bucket");
        exit(EXIT_FAILURE);
    }
    case 1: {
        cecs_flatbucket8_set_count(bucket, 0);
        break;
    }
    default: {
        const uint_fast8_t last_index = bucket_value_count - 1;
        const void *last_value = cecs_flatbucket8_get_value(bucket, position, bucket_value_count, value_size);
        void *const value = cecs_flatbucket8_get_value_mut(bucket, position, bucket_value_count, value_size);
        memcpy(value, last_value, value_size);
        cecs_flatbucket8_set_count(bucket, last_index);
        break;
    }
    }
}
static void cecs_flatbucket8_remove_expect(
    cecs_flatbucket8 *bucket,
    const uint_fast8_t position,
    const size_t value_size
) {
    const uint_fast8_t bucket_value_count = cecs_flatbucket8_get_count(*bucket);
    if (cecs_flatbucket8_get_index(*bucket, position) >= bucket_value_count) {
        assert(false && "error: cecs_flatbucket8_remove_expect called with out of bounds index");
        exit(EXIT_FAILURE);
    }

    const uint_fast8_t last_position = cecs_flatbucket8_find_position(*bucket, bucket_value_count - 1);
    const uint_fast8_t removed_index = cecs_flatbucket8_get_index(*bucket, position);
    cecs_flatbucket8_swap_last_pop(bucket, position, bucket_value_count, value_size);
    cecs_flatbucket8_set_index(bucket, position, cecs_flatbucket8_max_count - 1);
    cecs_flatbucket8_set_index(bucket, last_position, removed_index);
}

static uint_fast8_t cecs_flatbucket8_find_empty_position(const cecs_flatbucket8 bucket) {
    if (cecs_flatbucket8_is_full(bucket)) {
        assert(false && "error: cecs_flatbucket8_find_empty called on full bucket");
        exit(EXIT_FAILURE);
    }
    const uint_fast8_t empty_position = cecs_flatbucket8_find_position(bucket, cecs_flatbucket8_max_count - 1);
    if (empty_position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_find_empty found out of bounds position");
        exit(EXIT_FAILURE);
    }
    return empty_position;
}
static uint8_t cecs_flatbucket8_mark_hash_low_position(
    const cecs_flatbucket8 bucket,
    const cecs_flatset_hash_low_fast hash4
) {
    if (hash4 >= cecs_flatbucket8_hash4_max) {
        assert(false && "error: cecs_flatbucket8_find_hash_low_position called with out of bounds hash_low");
        exit(EXIT_FAILURE);
    }
    const uint8_t position_mark = cecs_mark_pattern_nibbles8_u4(bucket.hash_from_position8_b4, hash4);
    return position_mark;
}
static uint_fast8_t cecs_flatbucket8_find_hash_position(
    const cecs_flatbucket8 *bucket,
    const cecs_flatset_hash hash,
    const cecs_flatset_hash_low_fast hash4,
    const size_t hash_offset,
    const size_t hash_stride
) {
    const uint8_t position_mark = cecs_flatbucket8_mark_hash_low_position(*bucket, hash4);
    for (uint_fast8_t i = 0; i < CECS_FLATBUCKET8_POSITION_MAX; i++) {
        const cecs_flatset_hash *stored_hash = bucket->values + (i * hash_stride) + hash_offset;
        if (
            (position_mark & (1 << i))
            && (*stored_hash == hash)
        ) {
            return i;
        }
    }
    return CECS_FLATBUCKET8_POSITION_MAX;
}

static void cecs_flatbucket8_reset(cecs_flatbucket8 *bucket, const size_t value_size) {
    static const uint32_t index_from_position8_b4_default = 0x77777777;
    *bucket = (cecs_flatbucket8) {
        .hash_from_position8_b4 = 0,
        .index_from_position8_b4 = index_from_position8_b4_default
    };

    if (!CECS_ALLOC_FUNC_IS_ZERO_INIT) {
        memset(bucket->values, 0, cecs_flatbucket8_max_count * value_size);
    }
}

cecs_flatset cecs_flatset_create(void) {
    cecs_flatset set = {
        .buckets = NULL,
        .bucket_count = 0,
        .values_count = 0
    };
    return set;
}
cecs_flatset cecs_flatset_create_with_capacity(cecs_allocator *allocator, const size_t bucket_count, const size_t value_size) {
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_create_with_capacity called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }

    const size_t values_count = bucket_count * CECS_FLATBUCKET8_MAX_COUNT;
    cecs_flatbucket8 *const buckets = cecs_allocator_alloc(
        allocator,
        bucket_count * cecs_flatset_bucket_size(value_size)
    );
    cecs_flatset set = {
        .buckets = buckets,
        .bucket_count = bucket_count,
        .values_count = values_count
    };
    for (size_t i = 0; i < bucket_count; i++) {
        cecs_flatbucket8 *bucket = cecs_flatset_get_bucket_mut(&set, i, value_size);
        cecs_flatbucket8_reset(bucket, value_size);
    }
    return set;
}

void cecs_flatset_clear(cecs_flatset *set, const size_t value_size) {
    for (size_t i = 0; i < set->bucket_count; i++) {
        cecs_flatbucket8 *bucket = cecs_flatset_get_bucket_mut(set, i, value_size);
        cecs_flatbucket8_reset(bucket, value_size);
    }
    set->values_count = 0;
}
void cecs_flatset_destroy(cecs_flatset *set, cecs_allocator *allocator, const size_t value_size) {
    if (set->buckets) {
        cecs_allocator_free(allocator, set->buckets, set->bucket_count * cecs_flatset_bucket_size(value_size));
        set->buckets = NULL;
    }
    set->bucket_count = 0;
    set->values_count = 0;
}

void cecs_flatset_copy(
    cecs_flatset *destination,
    const cecs_flatset *source,
    cecs_allocator *allocator,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    if (cecs_flatset_capacity(destination) < cecs_flatset_count(source)) {
        cecs_flatset_destroy(destination, allocator, value_size);
        *destination = cecs_flatset_create_with_capacity(allocator, cecs_flatset_bucket_count(source), value_size);
    }

    for (size_t i = 0; i < source->bucket_count; ++i) {
        const cecs_flatbucket8 *source_bucket = cecs_flatset_get_bucket(source, i, value_size);
        const size_t bucket_value_count = cecs_flatbucket8_get_count(*source_bucket);
        for (uint_fast8_t j = 0; j < bucket_value_count; ++j) {
            const uint8_t *const source_value = cecs_flatbucket8_get_value_by_index(source_bucket, j, value_size);
            void *const destination_value = cecs_flatset_insert_within_expect(
                destination,
                *(const cecs_flatset_hash *)(source_value + hash_offset),
                value_size,
                hash_offset,
                hash_stride
            );
            memcpy(destination_value, source_value, value_size);
        }
    }
    if (cecs_flatset_count(destination) != cecs_flatset_count(source)) {
        assert(false && "error: cecs_flatset_copy did not copy the expected number of values");
        exit(EXIT_FAILURE);
    }
}
void cecs_flatset_resize(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    cecs_flatset new_set = cecs_flatset_create_with_capacity(
        allocator,
        new_bucket_count,
        value_size
    );
    cecs_flatset_copy(&new_set, set, allocator, value_size, hash_offset, hash_stride);
    cecs_flatset_destroy(set, allocator, value_size);
    *set = new_set;
}
void cecs_flatset_shrink(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    const size_t clamped_bucket_count = cecs_max(cecs_flatset_count(set) >> 3, new_bucket_count);
    cecs_flatset_resize(
        set,
        allocator,
        clamped_bucket_count,
        value_size,
        hash_offset,
        hash_stride
    );
}

cecs_flatbucket8 *cecs_flatset_find_insert_bucket_expect(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    const cecs_flatset_hash_low_fast hash4 = hash & CECS_FLATBUCKET8_HASH4_MASK;
    const size_t bucket_count = cecs_flatset_bucket_count(set);
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_find_insert_bucket_expect called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }

    const size_t bucket_count_mask = bucket_count- 1; 
    const size_t initial_bucket_index =
        (hash >> CECS_FLATBUCKET8_HASH4_MAX_LOG2) & (bucket_count_mask);
    size_t next_bucket_index = initial_bucket_index;
    cecs_flatbucket *bucket;
    cecs_flatbucket *insert;
    do {
        bucket = cecs_flatset_get_bucket_mut(set, next_bucket_index, value_size);
        const uint_fast8_t position = cecs_flatbucket8_find_hash_position(bucket, hash, hash4, hash_offset, hash_stride);
        if (position < CECS_FLATBUCKET8_MAX_COUNT) {
            assert(false && "error: cecs_flatset_insert_within_expect called with already occupied hash_low");
            exit(EXIT_FAILURE);
        } else if (!cecs_flatbucket8_is_full(*bucket)) {
            insert = bucket;
        }
        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while (
        (next_bucket_index != initial_bucket_index)
        && cecs_flatbucket8_has_been_full(*bucket)
    );

    if (cecs_flatbucket8_is_full(*insert)) {
        assert(false && "fatal error: cecs_flatset_insert_within_expect called on full bucket");
        exit(EXIT_FAILURE);
    }
    return insert;
}
cecs_flatbucket8 *cecs_flatset_find_insert_bucket(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    uint_fast8_t *out_position
) {
    const cecs_flatset_hash_low_fast hash4 = hash & CECS_FLATBUCKET8_HASH4_MASK;
    const size_t bucket_count = cecs_flatset_bucket_count(set);
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_find_insert_bucket_expect called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }
    
    const size_t bucket_count_mask = bucket_count- 1; 
    const size_t initial_bucket_index =
        (hash >> CECS_FLATBUCKET8_HASH4_MAX_LOG2) & (bucket_count_mask);
    size_t next_bucket_index = initial_bucket_index;
    cecs_flatbucket *bucket;
    cecs_flatbucket *insert;
    do {
        bucket = cecs_flatset_get_bucket_mut(set, next_bucket_index, value_size);
        const uint_fast8_t position = cecs_flatbucket8_find_hash_position(bucket, hash, hash4, hash_offset, hash_stride);
        if (position < CECS_FLATBUCKET8_POSITION_MAX) {
            *out_position = position;
            return bucket;
        } else if (!cecs_flatbucket8_is_full(*bucket)) {
            insert = bucket;
        }

        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while (
        (next_bucket_index != initial_bucket_index)
        && cecs_flatbucket8_has_been_full(*bucket)
    );
    *out_position = cecs_flatbucket8_find_empty_position(*insert);
    return insert;
}

bool cecs_flatset_find_bucket(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    const cecs_flatbucket **out_bucket,
    uint_fast8_t *out_position
) {
    const cecs_flatset_hash_low_fast hash4 = hash & CECS_FLATBUCKET8_HASH4_MASK;
    const size_t bucket_count = cecs_flatset_bucket_count(set);
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_find_insert_bucket_expect called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }
    
    const size_t bucket_count_mask = bucket_count- 1;
    const size_t initial_bucket_index =
        (hash >> CECS_FLATBUCKET8_HASH4_MAX_LOG2) & (bucket_count_mask);
    size_t next_bucket_index = initial_bucket_index;
    const cecs_flatbucket *bucket;
    do {
        bucket = cecs_flatset_get_bucket(set, next_bucket_index, value_size);
        const uint_fast8_t position = cecs_flatbucket8_find_hash_position(bucket, hash, hash4, hash_offset, hash_stride);
        const uint_fast8_t bucket_value_count = cecs_flatbucket8_get_count(*bucket);
        if (position < bucket_value_count) {
            *out_bucket = bucket;
            *out_position = position;
            return true;
        }
        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while ((next_bucket_index != initial_bucket_index) && cecs_flatbucket8_has_been_full(*bucket));
    
    *out_bucket = NULL;
    *out_position = CECS_FLATBUCKET8_POSITION_MAX;
    return false;
}
bool cecs_flatset_find_bucket_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    cecs_flatbucket **out_bucket,
    uint_fast8_t *out_position
) {
    const cecs_flatset_hash_low_fast hash4 = hash & CECS_FLATBUCKET8_HASH4_MASK;
    const size_t bucket_count = cecs_flatset_bucket_count(set);
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_find_insert_bucket_expect called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }
    
    const size_t bucket_count_mask = bucket_count- 1;
    const size_t initial_bucket_index =
        (hash >> CECS_FLATBUCKET8_HASH4_MAX_LOG2) & (bucket_count_mask);
    size_t next_bucket_index = initial_bucket_index;
    cecs_flatbucket *bucket;
    do {
        bucket = cecs_flatset_get_bucket_mut(set, next_bucket_index, value_size);
        const uint_fast8_t position = cecs_flatbucket8_find_hash_position(bucket, hash, hash4, hash_offset, hash_stride);
        if (position < CECS_FLATBUCKET8_POSITION_MAX) {
            *out_bucket = bucket;
            *out_position = position;
            return true;
        }
        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while ((next_bucket_index != initial_bucket_index) && cecs_flatbucket8_has_been_full(*bucket));
    
    *out_bucket = NULL;
    *out_position = CECS_FLATBUCKET8_POSITION_MAX;
    return false;
}

bool cecs_flatset_find(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    const void **out_value
) {
    const cecs_flatbucket8 *bucket;
    uint_fast8_t position;
    if (cecs_flatset_find_bucket(
        set,
        hash,
        value_size,
        hash_offset,
        hash_stride,
        &bucket,
        &position
    )) {
        *out_value = cecs_flatbucket8_get_value(bucket, position, cecs_flatbucket8_get_count(*bucket), value_size);
        return true;
    }
    return false;
}
bool cecs_flatset_find_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride,
    void **out_value
) {
    cecs_flatbucket8 *bucket;
    uint_fast8_t position;
    if (cecs_flatset_find_bucket_mut(
        set,
        hash,
        value_size,
        hash_offset,
        hash_stride,
        &bucket,
        &position
    )) {
        *out_value = cecs_flatbucket8_get_value_mut(bucket, position, cecs_flatbucket8_get_count(*bucket), value_size);
        return true;
    }
    return false;
}
const void *cecs_flatset_find_expect(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    const void *out_value;
    if (!cecs_flatset_find(set, hash, value_size, hash_offset, hash_stride, &out_value)) {
        assert(false && "error: cecs_flatset_find_expect called with non-existing hash");
        exit(EXIT_FAILURE);    
    }
    return out_value;
}
const void *cecs_flatset_find_expect_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    void *out_value;
    if (!cecs_flatset_find_mut(set, hash, value_size, hash_offset, hash_stride, &out_value)) {
        assert(false && "error: cecs_flatset_find_expect_mut called with non-existing hash");
        exit(EXIT_FAILURE);    
    }
    return out_value;
}

#define CECS_FLATSET_FULL_LOAD_FACTOR_TENTH 10
#ifndef CECS_FLATSET_MAX_LOAD_FACTOR_TENTH
#define CECS_FLATSET_MAX_LOAD_FACTOR_TENTH_DEFAULT 8
#define CECS_FLATSET_MAX_LOAD_FACTOR_TENTH CECS_FLATSET_MAX_LOAD_FACTOR_TENTH_DEFAULT
#endif
static_assert(
    CECS_FLATSET_MAX_LOAD_FACTOR_TENTH > 0 && CECS_FLATSET_MAX_LOAD_FACTOR_TENTH <= CECS_FLATSET_FULL_LOAD_FACTOR_TENTH,
    "CECS_FLATSET_MAX_LOAD_FACTOR_TENTH must be between 1 and 10"
);
#ifndef CECS_FLATSET_MIN_LOAD_FACTOR_TENTH
#define CECS_FLATSET_MIN_LOAD_FACTOR_TENTH_DEFAULT 2
#define CECS_FLATSET_MIN_LOAD_FACTOR_TENTH CECS_FLATSET_MIN_LOAD_FACTOR_TENTH_DEFAULT
#endif
static_assert(
    CECS_FLATSET_MIN_LOAD_FACTOR_TENTH > 0 && CECS_FLATSET_MIN_LOAD_FACTOR_TENTH <= CECS_FLATSET_FULL_LOAD_FACTOR_TENTH,
    "CECS_FLATSET_MIN_LOAD_FACTOR_TENTH must be between 1 and 10"
);

void *cecs_flatset_insert_into_bucket_expect(
    cecs_flatset *set,
    cecs_flatbucket8 *bucket,
    const cecs_flatset_hash hash,
    const size_t value_size
) {
    const uint_fast8_t position = cecs_flatbucket_find_empty_position(*bucket);
    const cecs_flatset_hash_low_fast hash4 = hash & CECS_FLATBUCKET8_HASH4_MASK;
    void *const value = cecs_flatbucket_insert_expect(bucket, position, hash4, value_size);
    ++set->values_count;
    return value;
}

void *cecs_flatset_insert_within_expect(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    // TODO: chain length
    if (cecs_flatset_count(set) >= cecs_flatset_capacity(set)) {
        assert(false && "error: cecs_flatset_insert_expect called on full set");
        exit(EXIT_FAILURE);
    }

    cecs_flatbucket8 *insert = cecs_flatset_find_insert_bucket_expect(
        set,
        hash,
        value_size,
        hash_offset,
        hash_stride
    );
    return cecs_flatset_insert_into_bucket_expect(
        set,
        insert,
        hash,
        value_size
    );
}
void *cecs_flatset_insert_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    if (((cecs_flatset_count(set) + 1) * CECS_FLATSET_FULL_LOAD_FACTOR_TENTH) >= (cecs_flatset_capacity(set) * CECS_FLATSET_MAX_LOAD_FACTOR_TENTH)) {
        cecs_flatset_resize(
            set,
            allocator,
            cecs_flatset_bucket_count(set) << 1,
            value_size,
            hash_offset,
            hash_stride
        );
    }
    return cecs_flatset_insert_within_expect(set, hash, value_size, hash_offset, hash_stride);
}
void *cecs_flatset_find_or_insert(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    if (((cecs_flatset_count(set) + 1) * CECS_FLATSET_FULL_LOAD_FACTOR_TENTH) >= (cecs_flatset_capacity(set) * CECS_FLATSET_MAX_LOAD_FACTOR_TENTH)) {
        cecs_flatset_resize(
            set,
            allocator,
            cecs_flatset_bucket_count(set) << 1,
            value_size,
            hash_offset,
            hash_stride
        );
    }

    uint_fast8_t position;
    cecs_flatbucket8 *insert = cecs_flatset_find_insert_bucket(
        set,
        hash,
        value_size,
        hash_offset,
        hash_stride,
        &position
    );
    const uint_fast8_t index = cecs_flatbucket8_get_index(*insert, position);
    const uint_fast8_t bucket_value_count = cecs_flatbucket8_get_count(*insert);
    if (index < bucket_value_count) {
        return cecs_flatbucket8_get_value(insert, position, bucket_value_count, value_size);
    } else {
        const cecs_flatset_hash_low_fast hash4 = hash & CECS_FLATBUCKET8_HASH4_MASK;
        void *const value = cecs_flatbucket8_insert_expect(insert, position, hash4, value_size);
        ++set->values_count;
        return value;
    }
}
void cecs_flatset_remove_from_bucket_stable_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    cecs_flatbucket *bucket,
    const uint_fast8_t position,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    if (position >= CECS_FLATBUCKET8_POSITION_MAX) {
        assert(false && "error: cecs_flatset_remove_from_bucket_expect called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    cecs_flatbucket8_remove_expect(bucket, position, value_size);
    --set->values_count;
}
void cecs_flatset_remove_from_bucket_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    cecs_flatbucket *bucket,
    const uint_fast8_t position,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    cecs_flatset_remove_from_bucket_stable_expect(
        set,
        allocator,
        bucket,
        position,
        value_size,
        hash_offset,
        hash_stride
    );
    if ((set->values_count * CECS_FLATSET_FULL_LOAD_FACTOR_TENTH) < (cecs_flatset_capacity(set) * CECS_FLATSET_MIN_LOAD_FACTOR_TENTH)) {
        cecs_flatset_shrink(
            set,
            allocator,
            cecs_flatset_bucket_count(set) >> 1,
            value_size,
            hash_offset,
            hash_stride
        );
    }
}
bool cecs_flatset_find_remove(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    cecs_flatbucket8 *bucket;
    uint_fast8_t position;
    if (cecs_flatset_find_bucket_mut(
        set,
        hash,
        value_size,
        hash_offset,
        hash_stride,
        &bucket,
        &position
    )) {
        if (position >= CECS_FLATBUCKET8_MAX_COUNT) {
            assert(false && "fatal error: cecs_flatset_find_bucket_mut returned out of bounds position");
            exit(EXIT_FAILURE);
        }
        cecs_flatset_remove_from_bucket_expect(
            set,
            allocator,
            bucket,
            position,
            value_size,
            hash_offset,
            hash_stride
        );
        return true;
    }    
    return false;
}
void cecs_flatset_find_remove_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    if (!cecs_flatset_find_remove(set, allocator, hash, value_size, hash_offset, hash_stride)) {
        assert(false && "error: cecs_flatset_remove_expect called with non-existing hash");
        exit(EXIT_FAILURE);
    }
}