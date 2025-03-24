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


extern inline uint64_t cecs_mark_2bit_runs_u64(uint64_t n);
extern inline uint32_t cecs_mark_2bit_runs_u32(uint32_t n);

extern inline uint64_t cecs_mark_4bit_runs_u64(uint64_t n);
extern inline uint32_t cecs_mark_4bit_runs_u32(uint32_t n);

uint64_t cecs_mark_dynamic_bit_runs_u64(uint64_t n, uint_fast8_t run_length);
uint32_t cecs_mark_dynamic_bit_runs_u32(uint32_t n, uint_fast8_t run_length);

uint64_t cecs_mark_dynamic_bit_runs_u64(uint64_t n, uint_fast8_t run_length) {
    uint64_t mask_even_odd[2];
    uint_fast8_t run_length_even_odd[2];
    while (run_length > 1) {
        run_length_even_odd[0] = run_length - 1;
        run_length_even_odd[1] = run_length >> 1;

        mask_even_odd[0] = n & (n >> 1);
        mask_even_odd[1] = n & (n >> run_length_even_odd[1]);

        const bool is_odd = run_length & 1;
        n &= mask_even_odd[is_odd];
        run_length = run_length_even_odd[is_odd];
    }
    return n;
}
uint32_t cecs_mark_dynamic_bit_runs_u32(uint32_t n, uint_fast8_t run_length) {
    uint32_t mask_even_odd[2];
    uint_fast8_t run_length_even_odd[2];
    while (run_length > 1) {
        run_length_even_odd[0] = run_length - 1;
        run_length_even_odd[1] = run_length >> 1;

        mask_even_odd[0] = n & (n >> 1);
        mask_even_odd[1] = n & (n >> run_length_even_odd[1]);

        const bool is_odd = run_length & 1;
        n &= mask_even_odd[is_odd];
        run_length = run_length_even_odd[is_odd];
    }
    return n;
}

extern inline uint64_t cecs_mark_bit_runs_u64(const uint64_t n, const uint_fast8_t run_length);
extern inline uint32_t cecs_mark_bit_runs_u32(const uint32_t n, const uint_fast8_t run_length);
extern inline size_t cecs_mark_bit_runs(const size_t n, const uint_fast8_t run_length);


extern inline uint16_t cecs_bitmask_u16(const uint_fast8_t start_bit, const uint_fast8_t length);
extern inline uint32_t cecs_bitmask_u32(const uint_fast8_t start_bit, const uint_fast8_t length);
extern inline uint64_t cecs_bitmask_u64(const uint_fast8_t start_bit, const uint_fast8_t length);
extern inline size_t cecs_bitmask(const uint_fast8_t start_bit, const uint_fast8_t length);