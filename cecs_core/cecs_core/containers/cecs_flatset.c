#include "cecs_flatset.h"
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>
#include <cecs_math/relations/cecs_ordering.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <memory.h>

const uint_fast8_t cecs_flatbucket_max_count = CECS_FLATBUCKET15_MAX_COUNT;

#define CECS_FLATBUCKET15_HASH4_MAX_LOG2 4
#define CECS_FLATBUCKET15_HASH4_MAX (1 << CECS_FLATBUCKET15_HASH4_MAX_LOG2)
#define CECS_FLATBUCKET15_HASH4_MASK (CECS_FLATBUCKET15_HASH4_MAX - 1)

static const cecs_flatset_hash_low_fast cecs_flatbucket_hash4_max = CECS_FLATBUCKET15_HASH4_MAX;


extern inline const void *cecs_flatbucket_get_value(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size);
extern inline void *cecs_flatbucket_get_value_mut(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size);

extern inline uint_fast8_t cecs_flatbucket_get_count(const cecs_flatbucket bucket);
static inline void cecs_flatbucket_set_count(cecs_flatbucket *bucket, const uint_fast8_t count) {
    if (count > CECS_FLATBUCKET15_MAX_COUNT) {
        assert(false && "error: cecs_flatbucket_set_count called with out of bounds count");
        exit(EXIT_FAILURE);
    }
    bucket->hash_from_index15_u4 &= ~0x0F;
    bucket->hash_from_index15_u4 |= count;
}
extern inline bool cecs_flatbucket_is_full(const cecs_flatbucket bucket);

static inline cecs_flatset_hash_low_fast cecs_flatbucket_get_hash_low(const cecs_flatbucket bucket, const uint_fast8_t index) {
    if (index >= cecs_flatbucket_max_count) {
        assert(false && "error: cecs_flatbucket_get_hash_low called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return (bucket.hash_from_index15_u4 >> ((index + 1) << 2)) & 0x0F;
}
static inline void cecs_flatbucket_set_hash_low(cecs_flatbucket *bucket, const uint_fast8_t index, const cecs_flatset_hash_low_fast hash4) {
    if (index >= cecs_flatbucket_max_count) {
        assert(false && "error: cecs_flatbucket_set_hash_low called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    if (hash4 >= cecs_flatbucket_hash4_max) {
        assert(false && "error: cecs_flatbucket_set_hash_low called with out of bounds hash4");
        exit(EXIT_FAILURE);
    }
    bucket->hash_from_index15_u4 &= ~(0x0F << ((index + 1) << 2));
    bucket->hash_from_index15_u4 |= (hash4 & 0x0F) << ((index + 1) << 2);
}

static inline bool cecs_flatbucket_has_been_full(const cecs_flatbucket *bucket, const size_t value_size) {
    return cecs_flatbucket_is_full(*bucket)
        || *(const bool *)cecs_flatbucket_get_value(bucket, cecs_flatbucket_max_count - 1, value_size);
}
static inline void cecs_flatbucket_set_been_full(cecs_flatbucket *bucket, const size_t value_size) {
    if (cecs_flatbucket_is_full(*bucket)) {
        assert(false && "error: cecs_flatbucket_set_been_full called on a full bucket. This would overwrite inserted values.");
        exit(EXIT_FAILURE);
    }
    bool *const been_full = cecs_flatbucket_get_value_mut(bucket, cecs_flatbucket_max_count - 1, value_size);
    *been_full = true;
}
static inline void cecs_flatbucket_unset_been_full(cecs_flatbucket *bucket, const size_t value_size) {
    if (cecs_flatbucket_is_full(*bucket)) {
        assert(false && "error: cecs_flatbucket_unset_been_full called on a full bucket. This would overwrite inserted values.");
        exit(EXIT_FAILURE);
    }
    bool *const been_full = cecs_flatbucket_get_value_mut(bucket, cecs_flatbucket_max_count - 1, value_size);
    *been_full = false;
}


static inline void *cecs_flatbucket_push(cecs_flatbucket *bucket, const size_t value_size) {
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*bucket);
    if (bucket_value_count > cecs_flatbucket_max_count) {
        assert(false && "error: cecs_flatbucket_push called with full bucket");
        exit(EXIT_FAILURE);
    }
    void *const value = &bucket->values[bucket_value_count * value_size];
    cecs_flatbucket_set_count(bucket, bucket_value_count + 1);
    return value;
}
static void *cecs_flatbucket_insert_expect(
    cecs_flatbucket *bucket,
    const uint_fast8_t index,
    const cecs_flatset_hash_low hash4,
    const size_t value_size
) {
    const uint_fast8_t index = cecs_flatbucket_get_index(*bucket, index);
    if (cecs_flatbucket_is_full(*bucket)) {
        assert(false && "error: cecs_flatbucket_insert_expect called with full bucket");
        exit(EXIT_FAILURE);
    }
    
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*bucket);
    if (index < bucket_value_count) {
        assert(false && "error: cecs_flatbucket_insert_expect called with already occupied hash_low");
        exit(EXIT_FAILURE);
    } else {
        cecs_flatbucket_set_hash_low(bucket, index, hash4);
        return cecs_flatbucket_push(bucket, value_size);
    }
}

static inline void cecs_flatbucket_swap_last_pop(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size) {
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*bucket);
    if (index >= bucket_value_count) {
        assert(false && "error: cecs_flatbucket_swap_last_pop called with out of bounds position");
        exit(EXIT_FAILURE);
    }

    switch (bucket_value_count) {
    case 0: {
        assert(false && "error: cecs_flatbucket_swap_last_pop called with empty bucket");
        exit(EXIT_FAILURE);
    }
    case 1: {
        cecs_flatbucket_set_count(bucket, 0);
        break;
    }
    default: {
        const uint_fast8_t last_index = bucket_value_count - 1;
        const void *last_value = cecs_flatbucket_get_value(bucket, index, value_size);
        void *const value = cecs_flatbucket_get_value_mut(bucket, index, value_size);
        memcpy(value, last_value, value_size);

        cecs_flatbucket_set_count(bucket, last_index);
        if (bucket_value_count == CECS_FLATBUCKET15_MAX_COUNT) {
            cecs_flatbucket_set_been_full(bucket, value_size);
        }
        break;
    }
    }
}
static void cecs_flatbucket_remove_expect(
    cecs_flatbucket *bucket,
    const uint_fast8_t index,
    const size_t value_size
) {
    cecs_flatbucket_swap_last_pop(bucket, index, value_size);
    const cecs_flatset_hash_low_fast last_hash = cecs_flatbucket_get_hash_low(*bucket, cecs_flatbucket_get_count(*bucket));
    cecs_flatbucket_set_hash_low(bucket, index, last_hash);
}

static uint8_t cecs_flatbucket_mark_hash_low_position(
    const cecs_flatbucket bucket,
    const cecs_flatset_hash_low_fast hash4
) {
    if (hash4 >= cecs_flatbucket_hash4_max) {
        assert(false && "error: cecs_flatbucket_mark_hash_low_position called with out of bounds hash_low");
        exit(EXIT_FAILURE);
    }
    const uint8_t position_mark = cecs_mark_pattern_nibbles8_u4(bucket.hash_from_position8_b4, hash4);
    return position_mark;
}

uint_fast8_t cecs_flatbucket_find_hash_position(
    const cecs_flatbucket *bucket,
    const cecs_flatset_hash hash,
    const cecs_flatset_hash_low_fast hash4,
    const size_t hash_offset,
    const size_t hash_stride
) {
    const uint8_t position_mark = cecs_flatbucket_mark_hash_low_position(*bucket, hash4);
    for (uint_fast8_t i = 0; i < CECS_FLATBUCKET8_POSITION_MAX; i++) {
        const cecs_flatset_hash *stored_hash = (const cecs_flatset_hash *)(bucket->values + (i * hash_stride) + hash_offset);
        if (
            (position_mark & (1 << i))
            && (*stored_hash == hash)
        ) {
            return i;
        }
    }
    return CECS_FLATBUCKET8_POSITION_MAX;
}

static void cecs_flatbucket_reset(cecs_flatbucket *bucket, const size_t value_size) {
    static const uint32_t index_from_position8_b4_default = 0x77777777;
    *bucket = (cecs_flatbucket) {
        .hash_from_position8_b4 = 0,
        .index_from_position8_b4 = index_from_position8_b4_default
    };

    if (!CECS_ALLOC_FUNC_IS_ZERO_INIT) {
        memset(bucket->values, 0, cecs_flatbucket_max_count * value_size);
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

    cecs_flatbucket *const buckets = cecs_allocator_alloc(
        allocator,
        bucket_count * cecs_flatset_bucket_size(value_size)
    );
    cecs_flatset set = {
        .buckets = buckets,
        .bucket_count = bucket_count,
        .values_count = 0
    };
    for (size_t i = 0; i < bucket_count; i++) {
        cecs_flatbucket *bucket = cecs_flatset_get_bucket_mut(&set, i, value_size);
        cecs_flatbucket_reset(bucket, value_size);
    }
    return set;
}

void cecs_flatset_clear(cecs_flatset *set, const size_t value_size) {
    for (size_t i = 0; i < set->bucket_count; i++) {
        cecs_flatbucket *bucket = cecs_flatset_get_bucket_mut(set, i, value_size);
        cecs_flatbucket_reset(bucket, value_size);
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

void cecs_flatset_extend_exclusive(
    cecs_flatset *destination,
    const cecs_flatset *source,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    const size_t destination_initial_count = cecs_flatset_count(destination);
    const size_t source_count = cecs_flatset_count(source);
    const size_t destination_remaining_capacity = cecs_flatset_capacity(destination) - destination_initial_count;
    if (destination_remaining_capacity < source_count) {
        assert(false && "error: cecs_flatset_extend_exclusive called with insufficient capacity in destination");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < source->bucket_count; ++i) {
        const cecs_flatbucket *source_bucket = cecs_flatset_get_bucket(source, i, value_size);
        const size_t bucket_value_count = cecs_flatbucket_get_count(*source_bucket);
        for (uint_fast8_t j = 0; j < bucket_value_count; ++j) {
            const uint8_t *const source_value = cecs_flatbucket_get_value_by_index(source_bucket, j, bucket_value_count, value_size);
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
    if (cecs_flatset_count(destination) != (destination_initial_count + source_count)) {
        assert(false && "error: cecs_flatset_extend_exclusive did not extend the destination set correctly with the expected number of values");
        exit(EXIT_FAILURE);
    }
}
void cecs_flatset_extend(
    cecs_flatset *destination,
    const cecs_flatset *source,
    cecs_allocator *allocator,
    const size_t value_size,
    const size_t hash_offset,
    const size_t hash_stride
) {
    const size_t destination_initial_count = cecs_flatset_count(destination);
    const size_t source_count = cecs_flatset_count(source);
    const size_t destination_remaining_capacity = cecs_flatset_capacity(destination) - destination_initial_count;
    if (destination_remaining_capacity < source_count) {
        assert(false && "error: cecs_flatset_extend called with insufficient capacity in destination");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < source->bucket_count; ++i) {
        const cecs_flatbucket *source_bucket = cecs_flatset_get_bucket(source, i, value_size);
        const size_t bucket_value_count = cecs_flatbucket_get_count(*source_bucket);
        for (uint_fast8_t j = 0; j < bucket_value_count; ++j) {
            const uint8_t *const source_value = cecs_flatbucket_get_value_by_index(source_bucket, j, bucket_value_count, value_size);
            void *const destination_value = cecs_flatset_find_or_insert(
                destination,
                allocator,
                *(const cecs_flatset_hash *)(source_value + hash_offset),
                value_size,
                hash_offset,
                hash_stride
            );
            memcpy(destination_value, source_value, value_size);
        }
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
    cecs_flatset_extend_exclusive(&new_set, set, value_size, hash_offset, hash_stride);
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
    const size_t clamped_bucket_count = cecs_max((cecs_flatset_count(set) + 7) >> 3, new_bucket_count);
    cecs_flatset_resize(
        set,
        allocator,
        clamped_bucket_count,
        value_size,
        hash_offset,
        hash_stride
    );
}

cecs_flatbucket *cecs_flatset_find_insert_bucket_expect(
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
        const uint_fast8_t position = cecs_flatbucket_find_hash_position(bucket, hash, hash4, hash_offset, hash_stride);
        if (position < CECS_FLATBUCKET8_MAX_COUNT) {
            assert(false && "error: cecs_flatset_insert_within_expect called with already occupied hash_low");
            exit(EXIT_FAILURE);
        } else if (!cecs_flatbucket_is_full(*bucket)) {
            insert = bucket;
        }
        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while (
        (next_bucket_index != initial_bucket_index)
        && cecs_flatbucket_has_been_full(*bucket)
    );

    if (cecs_flatbucket_is_full(*insert)) {
        assert(false && "fatal error: cecs_flatset_insert_within_expect called on full bucket");
        exit(EXIT_FAILURE);
    }
    return insert;
}
cecs_flatbucket *cecs_flatset_find_insert_bucket(
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
        const uint_fast8_t position = cecs_flatbucket_find_hash_position(bucket, hash, hash4, hash_offset, hash_stride);
        if (position < CECS_FLATBUCKET8_POSITION_MAX) {
            *out_position = position;
            return bucket;
        } else if (!cecs_flatbucket_is_full(*bucket)) {
            insert = bucket;
        }

        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while (
        (next_bucket_index != initial_bucket_index)
        && cecs_flatbucket_has_been_full(*bucket)
    );
    *out_position = cecs_flatbucket_find_empty_position(*insert);
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
        const uint_fast8_t position = cecs_flatbucket_find_hash_position(bucket, hash, hash4, hash_offset, hash_stride);
        const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*bucket);
        if (position < bucket_value_count) {
            *out_bucket = bucket;
            *out_position = position;
            return true;
        }
        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while ((next_bucket_index != initial_bucket_index) && cecs_flatbucket_has_been_full(*bucket));
    
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
        const uint_fast8_t position = cecs_flatbucket_find_hash_position(bucket, hash, hash4, hash_offset, hash_stride);
        if (position < CECS_FLATBUCKET8_POSITION_MAX) {
            *out_bucket = bucket;
            *out_position = position;
            return true;
        }
        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while ((next_bucket_index != initial_bucket_index) && cecs_flatbucket_has_been_full(*bucket));
    
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
    const cecs_flatbucket *bucket;
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
        *out_value = cecs_flatbucket_get_value(bucket, position, cecs_flatbucket_get_count(*bucket), value_size);
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
    cecs_flatbucket *bucket;
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
        *out_value = cecs_flatbucket_get_value_mut(bucket, position, cecs_flatbucket_get_count(*bucket), value_size);
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
    cecs_flatbucket *bucket,
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

    cecs_flatbucket *insert = cecs_flatset_find_insert_bucket_expect(
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
    cecs_flatbucket *insert = cecs_flatset_find_insert_bucket(
        set,
        hash,
        value_size,
        hash_offset,
        hash_stride,
        &position
    );
    const uint_fast8_t index = cecs_flatbucket_get_index(*insert, position);
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*insert);
    if (index < bucket_value_count) {
        return cecs_flatbucket_get_value_mut(insert, position, bucket_value_count, value_size);
    } else {
        const cecs_flatset_hash_low_fast hash4 = hash & CECS_FLATBUCKET8_HASH4_MASK;
        void *const value = cecs_flatbucket_insert_expect(insert, position, hash4, value_size);
        ++set->values_count;
        return value;
    }
}
void cecs_flatset_remove_from_bucket_stable_expect(
    cecs_flatset *set,
    cecs_flatbucket *bucket,
    const uint_fast8_t position,
    const size_t value_size
) {
    if (position >= CECS_FLATBUCKET8_POSITION_MAX) {
        assert(false && "error: cecs_flatset_remove_from_bucket_expect called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    cecs_flatbucket_remove_expect(bucket, position, value_size);
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
        bucket,
        position,
        value_size
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
    cecs_flatbucket *bucket;
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