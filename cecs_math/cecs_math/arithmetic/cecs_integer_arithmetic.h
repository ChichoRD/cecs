#ifndef CECS_INTEGER_ARITHMETIC_H
#define CECS_INTEGER_ARITHMETIC_H

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <intrin.h>

#define CECS_UINT16_BITS_LOG2 4
#define CECS_UINT16_BITS (1 << CECS_UINT16_BITS_LOG2)

#define CECS_UINT32_BITS_LOG2 5
#define CECS_UINT32_BITS (1 << CECS_UINT32_BITS_LOG2)

#define CECS_UINT64_BITS_LOG2 6
#define CECS_UINT64_BITS (1 << CECS_UINT64_BITS_LOG2)

#if (SIZE_MAX == UINT16_MAX)
#define CECS_SIZE_T_BITS_LOG2 CECS_UINT16_BITS_LOG2
#define CECS_SIZE_T_BITS_VALUE 16

#elif (SIZE_MAX == UINT32_MAX)
#define CECS_SIZE_T_BITS_LOG2 CECS_UINT32_BITS_LOG2
#define CECS_SIZE_T_BITS_VALUE 32

#elif (SIZE_MAX == UINT64_MAX)
#define CECS_SIZE_T_BITS_LOG2 CECS_UINT64_BITS_LOG2
#define CECS_SIZE_T_BITS_VALUE 64

#else
#error TBD code CECS_SIZE_T_BITS

#endif

#define CECS_SIZE_T_BITS (1 << CECS_SIZE_T_BITS_LOG2)
static_assert(
    CECS_SIZE_T_BITS == CECS_SIZE_T_BITS_VALUE,
    "fatal error: CECS_SIZE_T_BITS does not match SIZE_MAX"
);
extern const uint8_t cecs_size_t_bits;

#define CECS_LZCNT_U64 __lzcnt64
#define CECS_LZCNT_U32 __lzcnt
#define CECS_LZCNT_U16 __lzcnt16

inline uint_fast8_t cecs_log2_u64(const uint64_t n) {
    assert(n != 0 && "error: log2 of 0 is undefined");
    return (uint_fast8_t)(CECS_UINT64_BITS - CECS_LZCNT_U64(n) - 1);
}
inline uint_fast8_t cecs_log2_u32(const uint32_t n) {
    assert(n != 0 && "error: log2 of 0 is undefined");
    return (uint_fast8_t)(CECS_UINT32_BITS - CECS_LZCNT_U32(n) - 1);
}
inline uint_fast8_t cecs_log2_u16(const uint16_t n) {
    assert(n != 0 && "error: log2 of 0 is undefined");
    return (uint_fast8_t)(CECS_UINT16_BITS - CECS_LZCNT_U16(n) - 1);
}
inline uint_fast8_t cecs_log2(const size_t n) {
#if (SIZE_MAX == UINT16_MAX)
    return cecs_log2_u16((uint32_t)n);

#elif (SIZE_MAX == UINT32_MAX)
    return cecs_log2_u32((uint32_t)n);

#elif (SIZE_MAX == UINT64_MAX)
    return cecs_log2_u64((uint64_t)n);

#else
    #error TBD code CECS_SIZE_T_BITS
    return 0;

#endif
}

inline bool cecs_is_pow2_u64(const uint64_t n) {
    return n && !(n & (n - 1));
}
inline bool cecs_is_pow2_u32(const uint32_t n) {
    return n && !(n & (n - 1));
}
inline bool cecs_is_pow2_u16(const uint16_t n) {
    return n && !(n & (n - 1));
}
inline bool cecs_is_pow2(const size_t n) {
    return n && !(n & (n - 1));
}

inline uint64_t cecs_align_to_pow2_u64(const uint64_t size, const uint64_t alignment) {
    assert(cecs_is_pow2_u64(alignment) && "error: alignment must be a power of two");
    return (size + alignment - 1) & ~(alignment - 1);
}
inline uint32_t cecs_align_to_pow2_u32(const uint32_t size, const uint32_t alignment) {
    assert(cecs_is_pow2_u32(alignment) && "error: alignment must be a power of two");
    return (size + alignment - 1) & ~(alignment - 1);
}
inline uint16_t cecs_align_to_pow2_u16(const uint16_t size, const uint16_t alignment) {
    assert(cecs_is_pow2_u16(alignment) && "error: alignment must be a power of two");
    return (size + alignment - 1) & ~(alignment - 1);
}

inline size_t cecs_align_to_pow2(const size_t size, const size_t alignment) {
#if (SIZE_MAX == UINT16_MAX)
    return cecs_align_to_pow2_u16((uint16_t)size, (uint16_t)alignment);
#elif (SIZE_MAX == UINT32_MAX)
    return cecs_align_to_pow2_u32((uint32_t)size, (uint32_t)alignment);
#elif (SIZE_MAX == UINT64_MAX)
    return cecs_align_to_pow2_u64((uint64_t)size, (uint64_t)alignment);
#else
    #error TBD code CECS_SIZE_T_BITS
    return 0;
#endif
}


inline bool cecs_is_aligned_to_pow2_u64(const uint64_t size, const uint64_t alignment) {
    assert(cecs_is_pow2_u64(alignment) && "error: alignment must be a power of two");
    return (size & (alignment - 1)) == 0;
}
inline bool cecs_is_aligned_to_pow2_u32(const uint32_t size, const uint32_t alignment) {
    assert(cecs_is_pow2_u32(alignment) && "error: alignment must be a power of two");
    return (size & (alignment - 1)) == 0;
}
inline bool cecs_is_aligned_to_pow2_u16(const uint16_t size, const uint16_t alignment) {
    assert(cecs_is_pow2_u16(alignment) && "error: alignment must be a power of two");
    return (size & (alignment - 1)) == 0;
}
inline bool cecs_is_aligned_to_pow2(const size_t size, const size_t alignment) {
#if (SIZE_MAX == UINT16_MAX)
    return cecs_is_aligned_to_pow2_u16((uint16_t)size, (uint16_t)alignment);
#elif (SIZE_MAX == UINT32_MAX)
    return cecs_is_aligned_to_pow2_u32((uint32_t)size, (uint32_t)alignment);
#elif (SIZE_MAX == UINT64_MAX)
    return cecs_is_aligned_to_pow2_u64((uint64_t)size, (uint64_t)alignment);
#else
    #error TBD code CECS_SIZE_T_BITS
    return 0;
#endif
}

#endif