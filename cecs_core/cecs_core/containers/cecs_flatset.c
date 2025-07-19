#include "cecs_flatset.h"
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <memory.h>

static const uint8_t cecs_flatbucket14_index_invalid = 0x0F;
static const cecs_flatbucket14_index_pair_u cecs_flatbucket14_index_pair_invalid = {
    .pair = {
        .index0 = cecs_flatbucket14_index_invalid,
        .index1 = cecs_flatbucket14_index_invalid
    }
};
static inline uint_fast8_t cecs_flatbucket14_pair_read(cecs_flatbucket14_index_pair_u *pair, const uint_fast8_t subindex) {
    const uint_fast8_t indices[2] = {
        pair->pair.index0,
        pair->pair.index1
    };
    return indices[subindex & 1];
}
static inline void cecs_flatbucket14_pair_write(cecs_flatbucket14_index_pair_u *pair, const uint_fast8_t subindex, const uint_fast8_t value) {
    const cecs_flatbucket14_index_pair_u pairs[2] = {
        { .value = (uint8_t)(value | (pair->pair.index1 << 4)) },
        { .value = (uint8_t)((value << 4) | pair->pair.index0) },
    };
    *pair = pairs[subindex & 1];
}
static inline bool cecs_flatbucket14_pair_is_index_valid(const cecs_flatbucket14_index_pair_u pair, const uint_fast8_t subindex) {
    const bool valid[2] = {
        pair.pair.index0 != cecs_flatbucket14_index_invalid,
        pair.pair.index1 != cecs_flatbucket14_index_invalid
    };
    return valid[subindex & 1];
}

#define CECS_FLATBUCKET14_MAX_COUNT 14
static const uint_fast8_t cecs_flatbucket14_max_count = CECS_FLATBUCKET14_MAX_COUNT;
static inline cecs_flatbucket14_index_pair_u cecs_flatbucket14_get_index_pair(const cecs_flatbucket14 *bucket, const uint_fast8_t hash_low) {
    if (hash_low >= cecs_flatbucket14_max_count) {
        assert(false && "error: cecs_flatbucket14_get_index_pair called with out of bounds hash");
        exit(EXIT_FAILURE);
    }
    return bucket->hash_to_indices[hash_low >> 1];
}
static inline cecs_flatbucket14_index_pair_u *cecs_flatbucket14_get_index_pair_mut(cecs_flatbucket14 *bucket, const uint_fast8_t hash_low) {
    if (hash_low >= cecs_flatbucket14_max_count) {
        assert(false && "error: cecs_flatbucket14_get_index_pair called with out of bounds hash");
        exit(EXIT_FAILURE);
    }
    return &bucket->hash_to_indices[hash_low >> 1];
}

static inline void *cecs_flatbucket14_get_value_mut(cecs_flatbucket14 *bucket, const uint_fast8_t index, const size_t value_size) {
    if (index >= bucket->count) {
        assert(false && "error: cecs_flatbucket14_get_value_mut called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return &bucket->values[index * value_size];
}
static inline const void *cecs_flatbucket14_get_value(const cecs_flatbucket14 *bucket, const uint_fast8_t index, const size_t value_size) {
    if (index >= bucket->count) {
        assert(false && "error: cecs_flatbucket14_get_value called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return &bucket->values[index * value_size];
}


static inline void *cecs_flatbucket_push(cecs_flatbucket14 *bucket, const size_t value_size) {
    if (bucket->count >= cecs_flatbucket14_max_count) {
        assert(false && "error: cecs_flatbucket_push called with full bucket");
        exit(EXIT_FAILURE);
    }
    void *const value = &bucket->values[bucket->count * value_size];
    ++bucket->count;
    return value;
}
static inline void *cecs_flatbucket14_insert_expect(cecs_flatbucket14 *bucket, const cecs_flatset_hash_type hash, const size_t value_size) {
    const uint_fast8_t hash_low = hash % CECS_FLATBUCKET14_MAX_COUNT;
    cecs_flatbucket14_index_pair_u *const pair = cecs_flatbucket14_get_index_pair_mut(bucket, hash_low);
    if (cecs_flatbucket14_pair_is_index_valid(*pair, hash_low)) {
        assert(false && "error: cecs_flatbucket14_insert_expect called with already occupied hash");
        exit(EXIT_FAILURE);
    } else if (bucket->count >= cecs_flatbucket14_max_count) {
        assert(false && "error: cecs_flatbucket14_insert_expect called with full bucket");
        exit(EXIT_FAILURE);
    } else {
        cecs_flatbucket14_pair_write(pair, hash_low, bucket->count);
        return cecs_flatbucket14_push(bucket, value_size);
    }
}
static inline void *cecs_flatbucket14_insert(cecs_flatbucket14 *bucket, const cecs_flatset_hash_type hash, const size_t value_size) {
    const uint_fast8_t hash_low = hash % CECS_FLATBUCKET14_MAX_COUNT;
    cecs_flatbucket14_index_pair_u *const pair = cecs_flatbucket14_get_index_pair_mut(bucket, hash_low);
    if (cecs_flatbucket14_pair_is_index_valid(*pair, hash_low)) {
        return cecs_flatbucket14_get_value_mut(bucket, cecs_flatbucket14_pair_read(pair, hash_low), value_size);
    } else if (bucket->count >= cecs_flatbucket14_max_count) {
        assert(false && "error: cecs_flatbucket14_insert_expect called with full bucket");
        exit(EXIT_FAILURE);
    } else {
        cecs_flatbucket14_pair_write(pair, hash_low, bucket->count);
        return cecs_flatbucket14_push(bucket, value_size);
    }
}

static inline void cecs_flatbucket14_swap_last_pop(cecs_flatbucket14 *bucket, const uint_fast8_t index, const size_t value_size) {
    switch (bucket->count) {
    case 0: {
        assert(false && "error: attempted to swap last element of empty cecs_flatbucket14");
        exit(EXIT_FAILURE);
    }
    case 1: {
        --bucket->count;
    }
    default: {
        void *const swapped = cecs_flatbucket14_get_value_mut(bucket, index, value_size);
        const void *const last = cecs_flatbucket14_get_value(bucket, bucket->count - 1, value_size);
        memcpy(swapped, last, value_size);
        --bucket->count;
    }
    }
}
static inline bool cecs_flatbucket14_remove(cecs_flatbucket14 *bucket, const cecs_flatset_hash_type hash, const size_t value_size) {
    const uint_fast8_t hash_low = hash % CECS_FLATBUCKET14_MAX_COUNT;
    cecs_flatbucket14_index_pair_u *const pair = cecs_flatbucket14_get_index_pair_mut(bucket, hash_low);
    if (!cecs_flatbucket14_pair_is_index_valid(*pair, hash_low)) {
        return false;
    } else {
        const uint_fast8_t index = cecs_flatbucket14_pair_read(pair, hash_low);
        cecs_flatbucket14_pair_write(pair, hash_low, cecs_flatbucket14_index_invalid);
        
        cecs_flatbucket14_swap_last_pop(bucket, index, value_size);
        return true;
    }
}
static inline void cecs_flatbucket14_remove_expect(cecs_flatbucket14 *bucket, const cecs_flatset_hash_type hash, const size_t value_size) {
    if (!cecs_flatbucket14_remove(bucket, hash, value_size)) {
        assert(false && "error: cecs_flatbucket14_remove_expect called with non-existent hash");
        exit(EXIT_FAILURE);
    }
}
