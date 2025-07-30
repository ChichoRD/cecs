#ifndef CECS_INTEGER_ARITHMETIC_H
#define CECS_INTEGER_ARITHMETIC_H

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <intrin.h>
#include <memory.h>

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
inline uint_fast8_t cecs_lzcnt_u64(const uint64_t n) {
    return (uint_fast8_t)CECS_LZCNT_U64(n);
}
inline uint_fast8_t cecs_lzcnt_u32(const uint32_t n) {
    return (uint_fast8_t)CECS_LZCNT_U32(n);
}
inline uint_fast8_t cecs_lzcnt_u16(const uint16_t n) {
    return (uint_fast8_t)CECS_LZCNT_U16(n);
}
inline uint_fast8_t cecs_lzcnt_u8(const uint8_t n) {
    return cecs_lzcnt_u16((uint16_t)n) - 8;
}
inline uint_fast8_t cecs_lzcnt(const size_t n) {
#if (SIZE_MAX == UINT16_MAX)
    return cecs_lzcnt_u16((uint16_t)n);
#elif (SIZE_MAX == UINT32_MAX)
    return cecs_lzcnt_u32((uint32_t)n);
#elif (SIZE_MAX == UINT64_MAX)
    return cecs_lzcnt_u64((uint64_t)n);
#else
    #error TBD code CECS_SIZE_T_BITS
    return 0;
#endif
}


inline uint_fast8_t cecs_log2_u64(const uint64_t n) {
    assert(n != 0 && "error: log2 of 0 is undefined");
    return (uint_fast8_t)(CECS_UINT64_BITS - cecs_lzcnt_u64(n) - 1);
}
inline uint_fast8_t cecs_log2_u32(const uint32_t n) {
    assert(n != 0 && "error: log2 of 0 is undefined");
    return (uint_fast8_t)(CECS_UINT32_BITS - cecs_lzcnt_u32(n) - 1);
}
inline uint_fast8_t cecs_log2_u16(const uint16_t n) {
    assert(n != 0 && "error: log2 of 0 is undefined");
    return (uint_fast8_t)(CECS_UINT16_BITS - cecs_lzcnt_u16(n) - 1);
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


inline uint64_t cecs_mark_2bit_runs_u64(uint64_t n) {
    n &= n >> 1;
    return n;
}
inline uint32_t cecs_mark_2bit_runs_u32(uint32_t n) {
    n &= n >> 1;
    return n;
}

inline uint64_t cecs_mark_4bit_runs_u64(uint64_t n) {
    n &= n >> 2;
    n &= n >> 1;
    return n;
}
inline uint32_t cecs_mark_4bit_runs_u32(uint32_t n) {
    n &= n >> 2;
    n &= n >> 1;
    return n;
}

inline uint64_t cecs_mark_8bit_runs_u64(uint64_t n) {
    n &= n >> 4;
    n &= n >> 2;
    n &= n >> 1;
    return n;
}
inline uint32_t cecs_mark_8bit_runs_u32(uint32_t n) {
    n &= n >> 4;
    n &= n >> 2;
    n &= n >> 1;
    return n;
}

uint64_t cecs_mark_dynamic_bit_runs_u64(uint64_t n, uint_fast8_t run_length);
uint32_t cecs_mark_dynamic_bit_runs_u32(uint32_t n, uint_fast8_t run_length);

inline uint64_t cecs_mark_bit_runs_u64(const uint64_t n, const uint_fast8_t run_length) {
    switch (run_length) {
    case 0:
        return n;
    case 1:
        return n;
    case 2:
        return cecs_mark_2bit_runs_u64(n);
    case 4:
        return cecs_mark_4bit_runs_u64(n);
    case 8:
        return cecs_mark_8bit_runs_u64(n);
    default:
        return cecs_mark_dynamic_bit_runs_u64(n, run_length);
    }
}
inline uint32_t cecs_mark_bit_runs_u32(const uint32_t n, const uint_fast8_t run_length) {
    switch (run_length) {
    case 0:
        return n;
    case 1:
        return n;
    case 2:
        return cecs_mark_2bit_runs_u32(n);
    case 4:
        return cecs_mark_4bit_runs_u32(n);
    case 8:
        return cecs_mark_8bit_runs_u32(n);
    default:
        return cecs_mark_dynamic_bit_runs_u32(n, run_length);
    }
}

inline uint8_t cecs_wraparound_lsl_u8(const uint8_t value, const uint_fast8_t shift) {
    assert(shift < 8 && "error: shift must be less than 8");
    return (value << shift) | (value >> (8 - shift));
}
inline uint16_t cecs_wraparound_lsl_u16(const uint16_t value, const uint_fast8_t shift) {
    assert(shift < 16 && "error: shift must be less than 16");
    return (value << shift) | (value >> (16 - shift));
}
inline uint32_t cecs_wraparound_lsl_u32(const uint32_t value, const uint_fast8_t shift) {
    assert(shift < 32 && "error: shift must be less than 32");
    return (value << shift) | (value >> (32 - shift));
}
inline uint64_t cecs_wraparound_lsl_u64(const uint64_t value, const uint_fast8_t shift) {
    assert(shift < 64 && "error: shift must be less than 64");
    return (value << shift) | (value >> (64 - shift));
}
inline size_t cecs_wraparound_lsl(const size_t value, const uint_fast8_t shift) {
#if (SIZE_MAX == UINT16_MAX)
    return cecs_wraparound_lsl_u16((uint16_t)value, shift);
#elif (SIZE_MAX == UINT32_MAX)
    return cecs_wraparound_lsl_u32((uint32_t)value, shift);
#elif (SIZE_MAX == UINT64_MAX)
    return cecs_wraparound_lsl_u64((uint64_t)value, shift);
#else
    #error TBD code CECS_SIZE_T_BITS
    return 0;
#endif
}

inline uint8_t cecs_wraparound_lsr_u8(const uint8_t value, const uint_fast8_t shift) {
    assert(shift < 8 && "error: shift must be less than 8");
    return (value >> shift) | (value << (8 - shift));
}
inline uint16_t cecs_wraparound_lsr_u16(const uint16_t value, const uint_fast8_t shift) {
    assert(shift < 16 && "error: shift must be less than 16");
    return (value >> shift) | (value << (16 - shift));
}
inline uint32_t cecs_wraparound_lsr_u32(const uint32_t value, const uint_fast8_t shift) {
    assert(shift < 32 && "error: shift must be less than 32");
    return (value >> shift) | (value << (32 - shift));
}
inline uint64_t cecs_wraparound_lsr_u64(const uint64_t value, const uint_fast8_t shift) {
    assert(shift < 64 && "error: shift must be less than 64");
    return (value >> shift) | (value << (64 - shift));
}
inline size_t cecs_wraparound_lsr(const size_t value, const uint_fast8_t shift) {
#if (SIZE_MAX == UINT16_MAX)
    return cecs_wraparound_lsr_u16((uint16_t)value, shift);
#elif (SIZE_MAX == UINT32_MAX)
    return cecs_wraparound_lsr_u32((uint32_t)value, shift);
#elif (SIZE_MAX == UINT64_MAX)
    return cecs_wraparound_lsr_u64((uint64_t)value, shift);
#else
    #error TBD code CECS_SIZE_T_BITS
    return 0;
#endif
}

inline uint8_t cecs_gather_zeroed_msb8_u8(const uint64_t vec) {
    return (uint8_t)((vec * 0x0002040810204081ull) >> (64 - 8));
}
inline uint8_t cecs_gather_zeroed_lsb8_u8(const uint64_t vec) {
    return (uint8_t)((vec * 0x0102040810204080ull) >> (64 - 8));
}
inline uint8_t cecs_gather_msb8_u8(const uint64_t vec) {
    return cecs_gather_zeroed_msb8_u8(
        vec & 0x8080808080808080ull
    );
}
inline uint8_t cecs_gather_lsb8_u8(const uint64_t vec) {
    return cecs_gather_zeroed_lsb8_u8(
        vec & 0x0101010101010101ull
    );
}

inline uint64_t cecs_scatter_msb8_u1(const uint8_t vec) {
    return (
        ((vec & 0x55) * 0x0102040810204080ull)
        | ((vec & 0xAA) * 0x0102040810204080ull)
    ) & 0x8080808080808080ull;
}
inline uint64_t cecs_scatter_lsb8_u1(const uint8_t vec) {
    return (
        ((vec & 0x55) * 0x0002040810204081ull)
        | ((vec & 0xAA) * 0x0002040810204081ull)
    ) & 0x0101010101010101ull;
}
static_assert(false, "TODO: extern inline definitions");

inline uint8_t cecs_mark_zero_bytes8_u8(const uint64_t vec) {
    #define CECS_MARK_ZERO_BYTES8_U8 0x7f7f7f7f'7f7f7f7full
    const uint64_t raised = (vec & CECS_MARK_ZERO_BYTES8_U8) + CECS_MARK_ZERO_BYTES8_U8;
    const uint64_t marked_scatter = ~(raised | vec | CECS_MARK_ZERO_BYTES8_U8) >> 0x7;
    return (uint8_t)((marked_scatter * 0x102040810204080ull) >> (64 - 8));
    #undef CECS_MARK_ZERO_BYTES8_U8
}
inline uint_fast8_t cecs_first_zero_byte8_u8(const uint64_t vec) {
    const uint8_t marked = cecs_mark_zero_bytes8_u8(vec);
    return cecs_lzcnt_u8(marked);
}

inline uint8_t cecs_mark_pattern_bytes8_u8(const uint64_t vec, const uint8_t pattern) {
    const uint64_t pattern_mask = ((uint64_t)pattern) * 0x0101010101010101ull;
    return cecs_mark_zero_bytes8_u8(vec ^ pattern_mask);
}
inline uint_fast8_t cecs_first_pattern_byte8_u8(const uint64_t vec, const uint8_t pattern) {
    const uint8_t marked = cecs_mark_pattern_bytes8_u8(vec, pattern);
    return cecs_lzcnt_u8(marked);
}

#endif