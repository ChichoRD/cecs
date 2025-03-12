#include "cecs_integer_arithmetic.h"

const uint8_t cecs_size_t_bits = CECS_SIZE_T_BITS;

extern inline uint_fast8_t cecs_log2_u64(const uint64_t n);
extern inline uint_fast8_t cecs_log2_u32(const uint32_t n);
extern inline uint_fast8_t cecs_log2_u16(const uint16_t n);
extern inline uint_fast8_t cecs_log2(const size_t n);

extern inline bool cecs_is_pow2_u64(const uint64_t n);
extern inline bool cecs_is_pow2_u32(const uint32_t n);
extern inline bool cecs_is_pow2_u16(const uint16_t n);
extern inline bool cecs_is_pow2(const size_t n);

extern inline uint64_t cecs_align_to_pow2_u64(const uint64_t size, const uint64_t alignment);
extern inline uint32_t cecs_align_to_pow2_u32(const uint32_t size, const uint32_t alignment);
extern inline uint16_t cecs_align_to_pow2_u16(const uint16_t size, const uint16_t alignment);
extern inline size_t cecs_align_to_pow2(const size_t size, const size_t alignment);

extern inline bool cecs_is_aligned_to_pow2_u64(const uint64_t size, const uint64_t alignment);
extern inline bool cecs_is_aligned_to_pow2_u32(const uint32_t size, const uint32_t alignment);
extern inline bool cecs_is_aligned_to_pow2_u16(const uint16_t size, const uint16_t alignment);
extern inline bool cecs_is_aligned_to_pow2(const size_t size, const size_t alignment);

