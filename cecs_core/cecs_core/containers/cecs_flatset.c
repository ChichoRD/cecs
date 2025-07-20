#include "cecs_flatset.h"
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <memory.h>

#define CECS_FLATBUCKET16_MAX_COUNT_LOG2 4
#define CECS_FLATBUCKET16_MAX_COUNT (1 << CECS_FLATBUCKET16_MAX_COUNT_LOG2)
static const uint_fast8_t cecs_flatbucket16_max_count = CECS_FLATBUCKET16_MAX_COUNT;

static inline uint_fast8_t cecs_flatset16_hash_low(const cecs_flatset_hash hash) {
    return (uint_fast8_t)(hash & (CECS_FLATBUCKET16_MAX_COUNT - 1));
}
static inline cecs_flatset_hash cecs_flatset16_hash_high(const cecs_flatset_hash hash) {
    return (cecs_flatset_hash)(hash >> CECS_FLATBUCKET16_MAX_COUNT_LOG2);
}

static const uint8_t cecs_flatbucket16_index_invalid = 0x0F;
static const cecs_flatbucket16_index_pair_u cecs_flatbucket16_index_pair_invalid = {
    .pair = {
        .index0 = cecs_flatbucket16_index_invalid,
        .index1 = cecs_flatbucket16_index_invalid
    }
};
static inline uint_fast8_t cecs_flatbucket16_pair_read(const cecs_flatbucket16_index_pair_u pair, const uint_fast8_t subindex) {
    const uint_fast8_t indices[2] = {
        pair.pair.index0,
        pair.pair.index1
    };
    return indices[subindex & 1];
}
static inline void cecs_flatbucket16_pair_write(cecs_flatbucket16_index_pair_u *pair, const uint_fast8_t subindex, const uint_fast8_t value) {
    const cecs_flatbucket16_index_pair_u pairs[2] = {
        { .value = (uint8_t)(value | (pair->pair.index1 << CECS_FLATBUCKET16_MAX_COUNT_LOG2)) },
        { .value = (uint8_t)((value << CECS_FLATBUCKET16_MAX_COUNT_LOG2) | pair->pair.index0) },
    };
    *pair = pairs[subindex & 1];
}
static inline bool cecs_flatbucket16_pair_is_index_valid(
    const cecs_flatbucket16_count_psl count,
    const cecs_flatbucket16_index_pair_u pair,
    const uint_fast8_t subindex
) {
    const uint8_t indices[2] = {
        pair.pair.index0,
        pair.pair.index1
    };
    return indices[subindex & 1] < count.count;
}


static void cecs_flatbucket16_reset(cecs_flatbucket16 *bucket) {
    memset(&bucket->count_psl, 0, sizeof(bucket->count_psl));
    memset(bucket->hash_to_indices, cecs_flatbucket16_index_invalid, sizeof(bucket->hash_to_indices));
}

static inline cecs_flatbucket16_index_pair_u cecs_flatbucket16_get_index_pair(const cecs_flatbucket16 *bucket, const uint_fast8_t hash_low) {
    if (hash_low >= cecs_flatbucket16_max_count) {
        assert(false && "error: cecs_flatbucket16_get_index_pair called with out of bounds hash");
        exit(EXIT_FAILURE);
    }
    return bucket->hash_to_indices[hash_low >> 1];
}
static inline cecs_flatbucket16_index_pair_u *cecs_flatbucket16_get_index_pair_mut(cecs_flatbucket16 *bucket, const uint_fast8_t hash_low) {
    if (hash_low >= cecs_flatbucket16_max_count) {
        assert(false && "error: cecs_flatbucket16_get_index_pair called with out of bounds hash");
        exit(EXIT_FAILURE);
    }
    return &bucket->hash_to_indices[hash_low >> 1];
}

static inline void *cecs_flatbucket16_get_value_mut(cecs_flatbucket16 *bucket, const uint_fast8_t index, const size_t value_size) {
    if (index >= bucket->count_psl.count) {
        assert(false && "error: cecs_flatbucket16_get_value_mut called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return &bucket->values[index * value_size];
}
static inline const void *cecs_flatbucket16_get_value(const cecs_flatbucket16 *bucket, const uint_fast8_t index, const size_t value_size) {
    if (index >= bucket->count_psl.count) {
        assert(false && "error: cecs_flatbucket16_get_value called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return &bucket->values[index * value_size];
}

static inline uint_fast8_t cecs_flatbucket16_count(const cecs_flatbucket16 bucket) {
    return bucket.values;
}
static inline uint_fast8_t cecs_flatbucket16_get_psl(const cecs_flatbucket16 bucket, const uint_fast8_t index) {
    if (index >= bucket.count_psl.count) {
        assert(false && "error: cecs_flatbucket16_get_psl called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    if (index < 4) {
        return bucket.count_psl.psl0_4 >> (index * 2);
    } else if (index < 8) {
        return bucket.count_psl.psl4_8[(index - 4) >> 1] >> ((index & 1) * 4);
    } else if (index < 12) {
        return bucket.count_psl.psl8_12[(index - 8) >> 1] >> ((index & 1) * 4);
    } else {
        return bucket.count_psl.psl12_16[(index - 12) >> 1] >> ((index & 1) * 4);
    }
}
static inline void cecs_flatbucket16_set_psl(cecs_flatbucket16 *bucket, const uint_fast8_t index, const uint_fast8_t psl) {
    if (index >= bucket->count_psl.count) {
        assert(false && "error: cecs_flatbucket16_set_psl called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    if (index < 4) {
        bucket->count_psl.psl0_4 |= psl << (index * 2);
    } else if (index < 8) {
        bucket->count_psl.psl4_8[(index - 4) >> 1] |= psl << ((index & 1) * 4);
    } else if (index < 12) {
        bucket->count_psl.psl8_12[(index - 8) >> 1] |= psl << ((index & 1) * 4);
    } else {
        bucket->count_psl.psl12_16[(index - 12) >> 1] |= psl << ((index & 1) * 4);
    }
}

static uint_fast8_t cecs_flatbucket16_find_lower_psl(
    const cecs_flatbucket16 *bucket,
    const uint_fast8_t hash_low,
    const uint_fast8_t psl
) {
    if (hash_low >= cecs_flatbucket16_max_count) {
        assert(false && "error: cecs_flatbucket16_find_lower_psl called with out of bounds hash");
        exit(EXIT_FAILURE);
    }
    for (uint_fast8_t i = 0; i < bucket->count_psl.count; ++i) {
        if (cecs_flatbucket16_get_psl(*bucket, i) < psl) {
            return i;
        }
    }
    return bucket->count_psl.count;
}

static void cecs_flatbucket16_swap(
    cecs_flatbucket16 *bucket,
    const uint_fast8_t hash_low0,
    const uint_fast8_t hash_low1,
    const size_t value_size,
    uint8_t temporary[const value_size]
) {
    if (hash_low0 >= cecs_flatbucket16_max_count || hash_low1 >= cecs_flatbucket16_max_count) {
        assert(false && "error: cecs_flatbucket16_swap called with out of bounds hash");
        exit(EXIT_FAILURE);
    }
    cecs_flatbucket16_index_pair_u *pair0 = cecs_flatbucket16_get_index_pair_mut(bucket, hash_low0);
    cecs_flatbucket16_index_pair_u *pair1 = cecs_flatbucket16_get_index_pair_mut(bucket, hash_low1);

    if (
        !cecs_flatbucket16_pair_is_index_valid(bucket->count_psl, *pair0, hash_low0)
        || !cecs_flatbucket16_pair_is_index_valid(bucket->count_psl, *pair1, hash_low1)
    ) {
        assert(false && "error: cecs_flatbucket16_swap called with invalid indices");
        exit(EXIT_FAILURE);
    }

    const uint_fast8_t index0 = cecs_flatbucket16_pair_read(*pair0, hash_low0);
    const uint_fast8_t index1 = cecs_flatbucket16_pair_read(*pair1, hash_low1);

    cecs_flatbucket16_pair_write(pair0, hash_low0, index1);
    cecs_flatbucket16_pair_write(pair1, hash_low1, index0);

    const void *value0 = cecs_flatbucket16_get_value(bucket, index0, value_size);
    const void *value1 = cecs_flatbucket16_get_value(bucket, index1, value_size);
    memcpy(temporary, value0, value_size);
    memcpy(cecs_flatbucket16_get_value_mut(bucket, index0, value_size), value1, value_size);
    memcpy(cecs_flatbucket16_get_value_mut(bucket, index1, value_size), temporary, value_size);

    const uint_fast8_t psl0 = cecs_flatbucket16_get_psl(*bucket, index0);
    const uint_fast8_t psl1 = cecs_flatbucket16_get_psl(*bucket, index1);
    cecs_flatbucket16_set_psl(bucket, index0, psl1);
    cecs_flatbucket16_set_psl(bucket, index1, psl0);
}



static inline void *cecs_flatbucket_push(cecs_flatbucket16 *bucket, const size_t value_size) {
    if (bucket->count_psl.count >= cecs_flatbucket16_max_count) {
        assert(false && "error: cecs_flatbucket_push called with full bucket");
        exit(EXIT_FAILURE);
    }
    void *const value = &bucket->values[bucket->count_psl.count * value_size];
    ++bucket->count_psl.count;
    return value;
}
static inline void *cecs_flatbucket16_insert_expect(cecs_flatbucket16 *bucket, const uint_fast8_t hash_low, const size_t value_size) {
    cecs_flatbucket16_index_pair_u *const pair = cecs_flatbucket16_get_index_pair_mut(bucket, hash_low);
    if (bucket->count_psl.count >= cecs_flatbucket16_max_count) {
        assert(false && "error: cecs_flatbucket16_insert_expect called with full bucket");
        exit(EXIT_FAILURE);
    } else if (cecs_flatbucket16_pair_is_index_valid(bucket->count_psl, *pair, hash_low)) {
        assert(false && "error: cecs_flatbucket16_insert_expect called with already occupied hash");
        exit(EXIT_FAILURE);
    } else {
        cecs_flatbucket16_pair_write(pair, hash_low, bucket->count_psl.count);
        return cecs_flatbucket16_push(bucket, value_size);
    }
}
static inline void *cecs_flatbucket16_insert(cecs_flatbucket16 *bucket, const uint_fast8_t hash_low, const size_t value_size) {
    cecs_flatbucket16_index_pair_u *const pair = cecs_flatbucket16_get_index_pair_mut(bucket, hash_low);
    if (bucket->count_psl.count >= cecs_flatbucket16_max_count) {
        assert(false && "error: cecs_flatbucket16_insert called with full bucket");
        exit(EXIT_FAILURE);
    } else if (cecs_flatbucket16_pair_is_index_valid(bucket->count_psl, *pair, hash_low)) {
        return cecs_flatbucket16_get_value_mut(bucket, cecs_flatbucket16_pair_read(*pair, hash_low), value_size);
    } else {
        cecs_flatbucket16_pair_write(pair, hash_low, bucket->count_psl.count);
        return cecs_flatbucket16_push(bucket, value_size);
    }
}

static inline void cecs_flatbucket16_swap_last_pop(cecs_flatbucket16 *bucket, const uint_fast8_t index, const size_t value_size) {
    switch (bucket->count_psl.count) {
    case 0: {
        assert(false && "error: attempted to swap last element of empty cecs_flatbucket16");
        exit(EXIT_FAILURE);
    }
    case 1: {
        --bucket->count_psl.count;
    }
    default: {
        void *const swapped = cecs_flatbucket16_get_value_mut(bucket, index, value_size);
        const void *const last = cecs_flatbucket16_get_value(bucket, bucket->count_psl.count - 1, value_size);
        memcpy(swapped, last, value_size);
        --bucket->count_psl.count;
    }
    }
}
static inline bool cecs_flatbucket16_remove(cecs_flatbucket16 *bucket, const uint_fast8_t hash_low, const size_t value_size) {
    cecs_flatbucket16_index_pair_u *const pair = cecs_flatbucket16_get_index_pair_mut(bucket, hash_low);
    if (cecs_flatbucket16_pair_is_index_valid(bucket->count_psl, *pair, hash_low)) {
        const uint_fast8_t index = cecs_flatbucket16_pair_read(*pair, hash_low);
        cecs_flatbucket16_pair_write(pair, hash_low, cecs_flatbucket16_index_invalid);
        
        cecs_flatbucket16_swap_last_pop(bucket, index, value_size);
        return true;
    } else {
        return false;
    }
}
static inline void cecs_flatbucket16_remove_expect(cecs_flatbucket16 *bucket, const cecs_flatset_hash hash, const size_t value_size) {
    if (!cecs_flatbucket16_remove(bucket, hash, value_size)) {
        assert(false && "error: cecs_flatbucket16_remove_expect called with non-existent hash");
        exit(EXIT_FAILURE);
    }
}


cecs_flatset cecs_flatset_create(void) {
    return (cecs_flatset){
        .buckets = NULL,
        .bucket_count = 0,
        .values_count = 0
    };
}
cecs_flatset cecs_flatset_create_with_capacity(cecs_allocator *allocator, const size_t bucket_count_log2, const size_t value_size) {
    const size_t bucket_count = 1 << bucket_count_log2;
    cecs_flatset set = {
        .buckets = cecs_allocator_alloc_aligned(
            allocator,
            bucket_count * (sizeof(cecs_flatbucket16) + value_size * cecs_flatbucket16_max_count),
            cecs_max_alignment_from_size(value_size)
        ),
        .bucket_count = bucket_count,
        .values_count = 0
    };
    for (size_t i = 0; i < bucket_count; ++i) {
        cecs_flatbucket16_reset(&set.buckets[i]);
    }
    return set;
}

void *cecs_flatset_insert_expect(cecs_flatset *set, cecs_allocator *allocator, const cecs_flatset_hash hash, const size_t value_size) {
    const uint_fast8_t hash_low = cecs_flatset16_hash_low(hash);
    const cecs_flatset_hash hash_high = cecs_flatset16_hash_high(hash);
    const size_t bucket_index_mask = set->bucket_count - 1;

    const size_t start_bucket_index = ((size_t)hash_high) & bucket_index_mask;
    if (start_bucket_index >= set->bucket_count) {
        assert(false && "fatal error: set bucket count was not a power of two");
        exit(EXIT_FAILURE);
    }

    size_t bucket_index = start_bucket_index;
    if (set->buckets[bucket_index].count_psl.count >= cecs_flatbucket16_max_count) {
        do {
            bucket_index = (bucket_index + 1) & bucket_index_mask;
        } while (
            set->buckets[bucket_index].count_psl.count >= cecs_flatbucket16_max_count
            && bucket_index != start_bucket_index
        );
        if (bucket_index == start_bucket_index) {
            assert(false && "fatal error: cecs_flatset_insert_expect called with full set");
            exit(EXIT_FAILURE);
        }
    }

    cecs_flatbucket16 *const bucket = &set->buckets[bucket_index];
    const cecs_flatbucket16_index_pair_u pair = cecs_flatbucket16_get_index_pair(bucket, hash_low);
    if (!cecs_flatbucket16_pair_is_index_valid(bucket->count_psl, pair, hash_low)) {
        return cecs_flatbucket16_insert_expect(bucket, hash_low, value_size);
    } else {
        typedef struct from_to_indices {
            uint8_t from : 4;
            uint8_t to : 4;
        } from_to_indices;
        from_to_indices movements[16];
        uint_fast8_t movement_count = 0;
        
        uint_fast8_t previous_movement_index = hash_low;
        uint_fast8_t i = hash_low;
        cecs_flatbucket16_index_pair_u current_pair = pair;
        while ((i < bucket->count_psl.count) && cecs_flatbucket16_pair_is_index_valid(bucket->count_psl, current_pair, i)) {
            while ((i < bucket->count_psl.count) && (cecs_flatbucket16_get_psl(*bucket, i) >= i)) {
                ++i;
            }
            if (i < bucket->count_psl.count) {
                movements[movement_count] = (from_to_indices){
                    .from = previous_movement_index,
                    .to = i
                };
                ++movement_count;
                previous_movement_index = i;
                current_pair = cecs_flatbucket16_get_index_pair(bucket, i);
            }
        }
        movements[movement_count] = (from_to_indices){
            .from = previous_movement_index,
            .to = i
        };
        ++movement_count;

        for (int_fast8_t j = movement_count - 1; j >= 0; --j) {
            const from_to_indices *const movement = &movements[j];
            cecs_flatbucket16_index_pair_u *const pair_mut = cecs_flatbucket16_get_index_pair_mut(bucket, movement->to);
            if (cecs_flatbucket16_pair_is_index_valid(bucket->count_psl, *pair_mut, movement->from)) {
                cecs_flatbucket16_pair_write(pair_mut, movement->to, cecs_flatbucket16_pair_read(*pair_mut, movement->from));
            } else {
                cecs_flatbucket16_pair_write(pair_mut, movement->to, cecs_flatbucket16_index_invalid);
            }
        }
    }
}
