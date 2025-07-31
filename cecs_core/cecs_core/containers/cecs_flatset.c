#include "cecs_flatset.h"
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>
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

static inline uint_fast8_t cecs_flatbucket_get_position(const cecs_flatbucket8 bucket, const uint_fast8_t index) {
    if (index >= cecs_flatbucket8_max_count) {
        assert(false && "error: cecs_flatbucket8_get_position called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    const uint8_t index_mark = cecs_mark_pattern_nibbles8_u4(bucket.index_from_position8_b4 & 0x77777777, index);
    if (!cecs_is_pow2_u8(index_mark)) {
        assert(false && "error: cecs_flatbucket8_get_position detected that two positions exist that map to the same index");
        exit(EXIT_FAILURE);
    }
    const uint_fast8_t position = cecs_lzcnt_u8(index_mark);
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

typedef struct cecs_flatbucket8_count_chain_length {
    uint_fast8_t count_chain_length;
} cecs_flatbucket8_count_chain_length;
static inline cecs_flatbucket8_count_chain_length cecs_flatbucket8_count_chain_length_create(const uint_fast8_t count, const uint_fast8_t chain_length) {
    if (count >= 16 || chain_length >= 16) {
        assert(false && "error: cecs_flatbucket8_count_chain_length_create called with out of bounds count or chain_length");
        exit(EXIT_FAILURE);
    }
    
    return (cecs_flatbucket8_count_chain_length) {
        .count_chain_length = ((chain_length & 0b00001111) << 4) | (count & 0b00001111)
    };
}
static inline uint_fast8_t cecs_flatbucket8_count_chain_length_get_count(const cecs_flatbucket8_count_chain_length count_chain_length) {
    return count_chain_length.count_chain_length & 0b00001111;
}
static inline uint_fast8_t cecs_flatbucket8_count_chain_length_get_length(const cecs_flatbucket8_count_chain_length count_chain_length) {
    return (count_chain_length.count_chain_length >> 4) & 0b00001111;
}

static inline cecs_flatbucket8_count_chain_length cecs_flatbucket8_get_count_chain_length(const cecs_flatbucket8 bucket) {
    return (cecs_flatbucket8_count_chain_length) {
        .count_chain_length = cecs_gather_msn8_u4(bucket.index_from_position8_b4)
    };
}
static inline void cecs_flatbucket8_set_count_chain_length(
    cecs_flatbucket8 *bucket,
    const cecs_flatbucket8_count_chain_length count_chain
) {
    bucket->index_from_position8_b4 &= 0x77777777;
    bucket->index_from_position8_b4 |= cecs_scatter_msn8_u1(count_chain.count_chain_length);
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
    if (chain_length >= 16) {
        assert(false && "error: cecs_flatbucket8_set_chain_length called with out of bounds chain_length");
        exit(EXIT_FAILURE);
    }
    bucket->index_from_position8_b4 &= 0x7777FFFF;
    bucket->index_from_position8_b4 |= cecs_scatter_msn8_u1(chain_length << 4);
}

uint_fast8_t cecs_flatbucket8_get_count(const cecs_flatbucket8 bucket) {
    const cecs_flatbucket8_count_chain_length count_chain_length = cecs_flatbucket8_get_count_chain_length(bucket);
    return cecs_flatbucket8_count_chain_length_get_count(count_chain_length);
}
extern inline bool cecs_flatbucket8_is_full(const cecs_flatbucket8 bucket);

static const void *cecs_flatbucket8_get_value(const cecs_flatbucket8 *bucket, const uint_fast8_t position, const uint_fast8_t bucket_value_count, const size_t value_size) {
    if (position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_get_value called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    const uint_fast8_t index = cecs_flatbucket8_get_index(*bucket, position);
    if (index >= bucket_value_count) {
        assert(false && "error: cecs_flatbucket8_get_value called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return &bucket->values[index * value_size];
}
static void *cecs_flatbucket8_get_value_mut(cecs_flatbucket8 *bucket, const uint_fast8_t position, const uint_fast8_t bucket_value_count, const size_t value_size) {
    if (position >= cecs_flatbucket8_position_max) {
        assert(false && "error: cecs_flatbucket8_get_value_mut called with out of bounds position");
        exit(EXIT_FAILURE);
    }
    const uint_fast8_t index = cecs_flatbucket8_get_index(*bucket, position);
    if (index >= bucket_value_count) {
        assert(false && "error: cecs_flatbucket8_get_value_mut called with out of bounds index");
        exit(EXIT_FAILURE);
    }
    return &bucket->values[index * value_size];
}


// static uint_fast8_t cecs_flatbucket8_find_insert_position_expect(
//     const cecs_flatbucket8 *bucket,
//     const cecs_flatset_hash hash,
//     const size_t value_hash_offset,
//     const size_t value_size
// ) {
//     const 
// }


// static inline void cecs_flatbucket8_meta_set_at_index(
//     cecs_flatbucket8 *bucket,
//     const uint_fast8_t index,
//     const uint_fast8_t psl,
//     const cecs_flatset_hash_low_fast hash4,
//     const uint_fast8_t position
// ) {
//     if (index >= cecs_flatbucket8_max_count) {
//         assert(false && "error: cecs_flatbucket8_meta_set_at_index called with out of bounds index");
//         exit(EXIT_FAILURE);
//     }
//     cecs_flatbucket8_set_psl(bucket, index, psl);
//     cecs_flatbucket8_set_hash_low(bucket, index, hash4);
//     cecs_flatbucket8_set_position(bucket, index, position);
// }
// static inline void cecs_flatbucket8_meta_reset(cecs_flatbucket8 *bucket, const uint_fast8_t index, const uint_fast8_t index_value) {
//     cecs_flatbucket8_set_index(bucket, cecs_flatbucket8_get_position(*bucket, index), index_value);
//     cecs_flatbucket8_meta_set_at_index(
//         bucket,
//         index,
//         CECS_FLATBUCKET8_POSITION_MAX - 1,
//         0,
//         CECS_FLATBUCKET8_POSITION_MAX - 1
//     );
// }
// static inline void cecs_flatbucket8_meta_copy(cecs_flatbucket8 *bucket, const uint_fast8_t destination_index, const uint_fast8_t source_index) {
//     if (destination_index >= cecs_flatbucket8_max_count || source_index >= cecs_flatbucket8_max_count) {
//         assert(false && "error: cecs_flatbucket8_meta_copy called with out of bounds index");
//         exit(EXIT_FAILURE);
//     }
//     const uint_fast8_t position = cecs_flatbucket8_get_position(*bucket, source_index);
//     const cecs_flatset_hash_low_fast hash4 = cecs_flatbucket8_get_hash_low(*bucket, source_index);
//     const uint_fast8_t psl3 = cecs_flatbucket8_get_psl(*bucket, source_index);
//     cecs_flatbucket8_meta_set_at_index(bucket, destination_index, psl3, hash4, position);
// }
// static inline void cecs_flatbucket8_meta_move(cecs_flatbucket8 *bucket, const uint_fast8_t destination_index, const uint_fast8_t source_index) {
//     cecs_flatbucket8_meta_copy(bucket, destination_index, source_index);
//     cecs_flatbucket8_meta_reset(bucket, source_index, destination_index);
// }


static inline void *cecs_flatbucket8_push(cecs_flatbucket8 *bucket, const uint_fast8_t bucket_value_count, const size_t value_size) {
    if (bucket_value_count >= cecs_flatbucket8_max_count) {
        assert(false && "error: cecs_flatbucket8_push called with full bucket");
        exit(EXIT_FAILURE);
    }
    void *const value = &bucket->values[bucket_value_count * value_size];
    cecs_flatbucket8_set_count(bucket, bucket_value_count + 1);
    return value;
}
void *cecs_flatbucket8_insert_expect(
    cecs_flatbucket8 *bucket,
    const uint_fast8_t position,
    const cecs_flatset_hash_low hash4,
    const size_t value_size
) {
    const uint_fast8_t index = cecs_flatbucket8_get_index(*bucket, position);
    if (cecs_flatbucket8_is_full(*bucket)) {
        assert(false && "error: cecs_flatbucket8_insert_expect called with full bucket");
        exit(EXIT_FAILURE);
    }
    
    const uint_fast8_t bucket_value_count = cecs_flatbucket8_get_count(*bucket);
    if (index < bucket_value_count) {
        assert(false && "error: cecs_flatbucket8_insert_expect called with already occupied hash_low");
        exit(EXIT_FAILURE);
    } else {
        cecs_flatbucket8_set_index(bucket, position, bucket_value_count);
        cecs_flatbucket8_set_hash_low(bucket, position, hash4);
        return cecs_flatbucket8_push(bucket, bucket_value_count, value_size);
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

    const uint8_t last_position_mark = cecs_mark_pattern_nibbles8_u4(bucket->index_from_position8_b4, bucket_value_count - 1);
    if (!cecs_is_pow2_u8(last_position_mark)) {
        assert(false && "error: cecs_flatbucket8_remove_expect detected an invalid state. Two positions are pointing to the same index");
        exit(EXIT_FAILURE);
    }

    const uint_fast8_t last_position = cecs_lzcnt_u8(last_position_mark);
    const uint_fast8_t removed_index = cecs_flatbucket8_get_index(*bucket, position);
    cecs_flatbucket8_swap_last_pop(bucket, position, bucket_value_count, value_size);
    cecs_flatbucket8_set_index(bucket, position, cecs_flatbucket8_max_count - 1);
    cecs_flatbucket8_set_index(bucket, last_position, removed_index);
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

cecs_flatset cecs_flatset_create(void)
{
    cecs_flatset set = {
        .buckets = NULL,
        .bucket_count = 0,
        .values_count = 0
    };
    return set;
}
cecs_flatset cecs_flatset_create_with_capacity(cecs_allocator *allocator, const size_t bucket_count_log2, const size_t value_size) {
    const size_t bucket_count = 1 << bucket_count_log2;
    const size_t values_count = bucket_count * CECS_FLATBUCKET8_MAX_COUNT;
    cecs_flatbucket8 *const buckets = cecs_allocator_alloc(
        allocator,
        bucket_count * sizeof(cecs_flatbucket8)
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

void *cecs_flatset_insert(cecs_flatset *set, cecs_allocator *allocator, const cecs_flatset_hash hash, const size_t value_size) {
    static_assert(false, "TODO: find func, both for empty spot for hash and for existing coinciding hash");
}
