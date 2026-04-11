#include "cecs_flatset.h"
#include <arithmetic/cecs_integer_arithmetic.h>
#include <relations/cecs_ordering.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <memory.h>

extern inline const void *cecs_flatset_bucket_get_value_unchecked(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size);
extern inline void *cecs_flatset_bucket_get_value_mut_unchecked(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size);
extern inline const void *cecs_flatset_bucket_get_value(const cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size);
extern inline void *cecs_flatset_bucket_get_value_mut(cecs_flatbucket *bucket, const uint_fast8_t index, const size_t value_size);

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

    uint8_t *const bucket_allocation = (uint8_t *)cecs_allocator_alloc_aligned(
        allocator,
        bucket_count * cecs_flatset_bucket_size(value_size),
        8
    );
    cecs_flatset set = {
        .buckets = (cecs_flatbucket *)bucket_allocation,
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
        uint8_t *const allocation = (uint8_t *)set->buckets;
        cecs_allocator_free(allocator, allocation, set->bucket_count * cecs_flatset_bucket_size(value_size));
        set->buckets = NULL;
    }
    set->bucket_count = 0;
    set->values_count = 0;
}

void cecs_flatset_extend_exclusive(
    cecs_flatset *destination,
    const cecs_flatset *source,
    const size_t value_size,
    const size_t hash_offset
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
        const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*source_bucket);
        for (uint_fast8_t j = 0; j < bucket_value_count; ++j) {
            const uint8_t *const source_value = cecs_flatset_bucket_get_value(source_bucket, j, value_size);
            void *const destination_value = cecs_flatset_insert_within_expect(
                destination,
                *(const cecs_flatset_hash *)(source_value + hash_offset),
                value_size,
                hash_offset
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
    const size_t hash_offset
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
        const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*source_bucket);
        for (uint_fast8_t j = 0; j < bucket_value_count; ++j) {
            const uint8_t *const source_value = cecs_flatset_bucket_get_value(source_bucket, j, value_size);
            void *const destination_value = cecs_flatset_find_or_insert(
                destination,
                allocator,
                *(const cecs_flatset_hash *)(source_value + hash_offset),
                value_size,
                hash_offset
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
    const size_t hash_offset
) {
    cecs_flatset new_set = cecs_flatset_create_with_capacity(
        allocator,
        new_bucket_count,
        value_size
    );
    cecs_flatset_extend_exclusive(&new_set, set, value_size, hash_offset);
    cecs_flatset_destroy(set, allocator, value_size);
    *set = new_set;
}
void cecs_flatset_shrink(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const size_t new_bucket_count,
    const size_t value_size,
    const size_t hash_offset
) {
    const size_t clamped_bucket_count = cecs_max((cecs_flatset_count(set) + 7) >> CECS_FLATBUCKET8_MAX_COUNT_LOG2, new_bucket_count);
    cecs_flatset_resize(
        set,
        allocator,
        clamped_bucket_count,
        value_size,
        hash_offset
    );
}

static inline size_t cecs_flatset_bucket_index_from_hash(const cecs_flatset_hash hash, const size_t bucket_count_mask) {
    assert(
        cecs_is_pow2(bucket_count_mask + 1)
        && "error: cecs_flatset_bucket_index_from_hash called with non power of two bucket count mask"
    );
    return (hash >> CECS_FLATBUCKET8_MAX_COUNT_LOG2) & bucket_count_mask;
}
cecs_flatbucket *cecs_flatset_find_insert_bucket_expect(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
) {
    const cecs_flatset_hash_low_fast hash7 = cecs_flatbucket_hash_low_get(hash);
    const size_t bucket_count = cecs_flatset_bucket_count(set);
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_find_insert_bucket_expect called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }

    const size_t bucket_count_mask = bucket_count - 1; 
    const size_t initial_bucket_index = cecs_flatset_bucket_index_from_hash(hash, bucket_count_mask);
    size_t next_bucket_index = initial_bucket_index;
    cecs_flatbucket *bucket;
    cecs_flatbucket *insert;
    do {
        bucket = cecs_flatset_get_bucket_mut(set, next_bucket_index, value_size);
        const uint_fast8_t index = cecs_flatbucket_find_hash_index(bucket, hash, hash7, hash_offset, value_size);
        if (index < CECS_FLATBUCKET8_MAX_COUNT) {
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
    uint_fast8_t *out_index
) {
    const cecs_flatset_hash_low_fast hash7 = cecs_flatbucket_hash_low_get(hash);
    const size_t bucket_count = cecs_flatset_bucket_count(set);
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_find_insert_bucket_expect called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }
    
    const size_t bucket_count_mask = bucket_count - 1; 
    const size_t initial_bucket_index = cecs_flatset_bucket_index_from_hash(hash, bucket_count_mask);
    size_t next_bucket_index = initial_bucket_index;
    cecs_flatbucket *bucket;
    cecs_flatbucket *insert;
    do {
        bucket = cecs_flatset_get_bucket_mut(set, next_bucket_index, value_size);
        const uint_fast8_t index = cecs_flatbucket_find_hash_index(bucket, hash, hash7, hash_offset, value_size);
        if (index < CECS_FLATBUCKET8_MAX_COUNT) {
            *out_index = index;
            return bucket;
        } else if (!cecs_flatbucket_is_full(*bucket)) {
            insert = bucket;
        }

        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while (
        (next_bucket_index != initial_bucket_index)
        && cecs_flatbucket_has_been_full(*bucket)
    );
    *out_index = cecs_flatbucket_get_count(*insert);
    return insert;
}

bool cecs_flatset_find_bucket(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const cecs_flatbucket **out_bucket,
    uint_fast8_t *out_index
) {
    const cecs_flatset_hash_low_fast hash7 = cecs_flatbucket_hash_low_get(hash);
    const size_t bucket_count = cecs_flatset_bucket_count(set);
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_find_insert_bucket_expect called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }
    
    const size_t bucket_count_mask = bucket_count - 1;
    const size_t initial_bucket_index = cecs_flatset_bucket_index_from_hash(hash, bucket_count_mask);
    size_t next_bucket_index = initial_bucket_index;
    const cecs_flatbucket *bucket;
    do {
        bucket = cecs_flatset_get_bucket(set, next_bucket_index, value_size);
        const uint_fast8_t index = cecs_flatbucket_find_hash_index(bucket, hash, hash7, hash_offset, value_size);
        if (index < CECS_FLATBUCKET8_MAX_COUNT) {
            *out_bucket = bucket;
            *out_index = index;
            return true;
        }
        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while ((next_bucket_index != initial_bucket_index) && cecs_flatbucket_has_been_full(*bucket));
    
    *out_bucket = NULL;
    *out_index = CECS_FLATBUCKET8_MAX_COUNT;
    return false;
}
bool cecs_flatset_find_bucket_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    cecs_flatbucket **out_bucket,
    uint_fast8_t *out_index
) {
    const cecs_flatset_hash_low_fast hash7 = cecs_flatbucket_hash_low_get(hash);
    const size_t bucket_count = cecs_flatset_bucket_count(set);
    if (!cecs_is_pow2(bucket_count)) {
        assert(false && "error: cecs_flatset_find_insert_bucket_expect called with non power of two bucket count");
        exit(EXIT_FAILURE);
    }
    
    const size_t bucket_count_mask = bucket_count - 1;
    const size_t initial_bucket_index = cecs_flatset_bucket_index_from_hash(hash, bucket_count_mask);
    size_t next_bucket_index = initial_bucket_index;
    cecs_flatbucket *bucket;
    do {
        bucket = cecs_flatset_get_bucket_mut(set, next_bucket_index, value_size);
        const uint_fast8_t index = cecs_flatbucket_find_hash_index(bucket, hash, hash7, hash_offset, value_size);
        if (index < CECS_FLATBUCKET8_MAX_COUNT) {
            *out_bucket = bucket;
            *out_index = index;
            return true;
        }
        next_bucket_index = (next_bucket_index + 1) & bucket_count_mask;
    } while ((next_bucket_index != initial_bucket_index) && cecs_flatbucket_has_been_full(*bucket));
    
    *out_bucket = NULL;
    *out_index = CECS_FLATBUCKET8_MAX_COUNT;
    return false;
}

bool cecs_flatset_find(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    const void **out_value
) {
    const cecs_flatbucket *bucket;
    uint_fast8_t index;
    if (cecs_flatset_find_bucket(
        set,
        hash,
        value_size,
        hash_offset,
        &bucket,
        &index
    )) {
        *out_value = cecs_flatset_bucket_get_value(bucket, index, value_size);
        return true;
    }
    return false;
}
bool cecs_flatset_find_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset,
    void **out_value
) {
    cecs_flatbucket *bucket;
    uint_fast8_t index;
    if (cecs_flatset_find_bucket_mut(
        set,
        hash,
        value_size,
        hash_offset,
        &bucket,
        &index
    )) {
        *out_value = cecs_flatset_bucket_get_value_mut(bucket, index, value_size);
        return true;
    }
    return false;
}
const void *cecs_flatset_find_expect(
    const cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
) {
    const void *out_value;
    if (!cecs_flatset_find(set, hash, value_size, hash_offset, &out_value)) {
        assert(false && "error: cecs_flatset_find_expect called with non-existing hash");
        exit(EXIT_FAILURE);    
    }
    return out_value;
}
const void *cecs_flatset_find_expect_mut(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
) {
    void *out_value;
    if (!cecs_flatset_find_mut(set, hash, value_size, hash_offset, &out_value)) {
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
    void *const value = cecs_flatbucket_insert_expect(bucket, cecs_flatbucket_get_count(*bucket), cecs_flatbucket_hash_low_get(hash), value_size, 0);
    ++set->values_count;
    return value;
}

void *cecs_flatset_insert_within_expect(
    cecs_flatset *set,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
) {
    if (cecs_flatset_count(set) >= cecs_flatset_capacity(set)) {
        assert(false && "error: cecs_flatset_insert_expect called on full set");
        exit(EXIT_FAILURE);
    }

    cecs_flatbucket *insert = cecs_flatset_find_insert_bucket_expect(
        set,
        hash,
        value_size,
        hash_offset
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
    const size_t hash_offset
) {
    if (((cecs_flatset_count(set) + 1) * CECS_FLATSET_FULL_LOAD_FACTOR_TENTH) >= (cecs_flatset_capacity(set) * CECS_FLATSET_MAX_LOAD_FACTOR_TENTH)) {
        cecs_flatset_resize(
            set,
            allocator,
            cecs_flatset_bucket_count(set) << 1,
            value_size,
            hash_offset
        );
    }
    return cecs_flatset_insert_within_expect(set, hash, value_size, hash_offset);
}
void *cecs_flatset_find_or_insert(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
) {
    if (((cecs_flatset_count(set) + 1) * CECS_FLATSET_FULL_LOAD_FACTOR_TENTH) >= (cecs_flatset_capacity(set) * CECS_FLATSET_MAX_LOAD_FACTOR_TENTH)) {
        cecs_flatset_resize(
            set,
            allocator,
            cecs_flatset_bucket_count(set) << 1,
            value_size,
            hash_offset
        );
    }

    uint_fast8_t index;
    cecs_flatbucket *insert = cecs_flatset_find_insert_bucket(
        set,
        hash,
        value_size,
        hash_offset,
        &index
    );
    const uint_fast8_t bucket_value_count = cecs_flatbucket_get_count(*insert);
    if (index < bucket_value_count) {
        return cecs_flatset_bucket_get_value_mut(insert, index, value_size);
    } else {
        const cecs_flatset_hash_low_fast hash7 = cecs_flatbucket_hash_low_get(hash);
        void *const value = cecs_flatbucket_insert_expect(insert, index, hash7, value_size, 0);
        ++set->values_count;
        return value;
    }
}
void cecs_flatset_remove_from_bucket_stable_expect(
    cecs_flatset *set,
    cecs_flatbucket *bucket,
    const uint_fast8_t index,
    const size_t value_size
) {
    if (index >= cecs_flatbucket_get_count(*bucket)) {
        assert(false && "error: cecs_flatset_remove_from_bucket_expect called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    cecs_flatbucket_remove_expect(bucket, index, value_size, 0);
    --set->values_count;
}
void cecs_flatset_remove_from_bucket_expect(
    cecs_flatset *set,
    cecs_allocator *allocator,
    cecs_flatbucket *bucket,
    const uint_fast8_t index,
    const size_t value_size,
    const size_t hash_offset
) {
    cecs_flatset_remove_from_bucket_stable_expect(
        set,
        bucket,
        index,
        value_size
    );
    if ((set->values_count * CECS_FLATSET_FULL_LOAD_FACTOR_TENTH) < (cecs_flatset_capacity(set) * CECS_FLATSET_MIN_LOAD_FACTOR_TENTH)) {
        cecs_flatset_shrink(
            set,
            allocator,
            cecs_flatset_bucket_count(set) >> 1,
            value_size,
            hash_offset
        );
    }
}
bool cecs_flatset_find_remove(
    cecs_flatset *set,
    cecs_allocator *allocator,
    const cecs_flatset_hash hash,
    const size_t value_size,
    const size_t hash_offset
) {
    cecs_flatbucket *bucket;
    uint_fast8_t index;
    if (cecs_flatset_find_bucket_mut(
        set,
        hash,
        value_size,
        hash_offset,
        &bucket,
        &index
    )) {
        if (index >= CECS_FLATBUCKET8_MAX_COUNT) {
            assert(false && "fatal error: cecs_flatset_find_bucket_mut returned out of bounds index");
            exit(EXIT_FAILURE);
        }
        cecs_flatset_remove_from_bucket_expect(
            set,
            allocator,
            bucket,
            index,
            value_size,
            hash_offset
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
    const size_t hash_offset
) {
    if (!cecs_flatset_find_remove(set, allocator, hash, value_size, hash_offset)) {
        assert(false && "error: cecs_flatset_remove_expect called with non-existing hash");
        exit(EXIT_FAILURE);
    }
}
