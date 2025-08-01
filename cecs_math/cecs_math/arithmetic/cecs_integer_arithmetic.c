#include "cecs_integer_arithmetic.h"

const uint8_t cecs_size_t_bits = CECS_SIZE_T_BITS;

extern inline uint_fast8_t cecs_lzcnt_u64(const uint64_t n);
extern inline uint_fast8_t cecs_lzcnt_u32(const uint32_t n);
extern inline uint_fast8_t cecs_lzcnt_u16(const uint16_t n);
extern inline uint_fast8_t cecs_lzcnt_u8(const uint8_t n);

extern inline uint_fast8_t cecs_tzcnt_u64(const uint64_t n);
extern inline uint_fast8_t cecs_tzcnt_u32(const uint32_t n);
extern inline uint_fast8_t cecs_tzcnt_u16(const uint16_t n);
extern inline uint_fast8_t cecs_tzcnt_u8(const uint8_t n);
extern inline uint_fast8_t cecs_tzcnt(const size_t n);


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

extern inline uint8_t cecs_wraparound_lsl_u8(const uint8_t value, const uint_fast8_t shift);
extern inline uint16_t cecs_wraparound_lsl_u16(const uint16_t value, const uint_fast8_t shift);
extern inline uint32_t cecs_wraparound_lsl_u32(const uint32_t value, const uint_fast8_t shift);
extern inline uint64_t cecs_wraparound_lsl_u64(const uint64_t value, const uint_fast8_t shift);
extern inline size_t cecs_wraparound_lsl(const size_t value, const uint_fast8_t shift);

extern inline uint8_t cecs_wraparound_lsr_u8(const uint8_t value, const uint_fast8_t shift);
extern inline uint16_t cecs_wraparound_lsr_u16(const uint16_t value, const uint_fast8_t shift);
extern inline uint32_t cecs_wraparound_lsr_u32(const uint32_t value, const uint_fast8_t shift);
extern inline uint64_t cecs_wraparound_lsr_u64(const uint64_t value, const uint_fast8_t shift);
extern inline size_t cecs_wraparound_lsr(const size_t value, const uint_fast8_t shift);

extern inline uint8_t cecs_gather_zeroed_msb8_u8(const uint64_t vec);
extern inline uint8_t cecs_gather_zeroed_lsb8_u8(const uint64_t vec);
extern inline uint8_t cecs_gather_msb8_u8(const uint64_t vec);
extern inline uint8_t cecs_gather_lsb8_u8(const uint64_t vec);

extern inline uint8_t cecs_gather_msn8_u4(const uint32_t vec);
extern inline uint8_t cecs_gather_lsn8_u4(const uint32_t vec);

extern inline uint64_t cecs_scatter_msb8_u1(const uint8_t vec);
extern inline uint64_t cecs_scatter_lsb8_u1(const uint8_t vec);

extern inline uint32_t cecs_scatter_msn8_u1(const uint8_t vec);
extern inline uint32_t cecs_scatter_lsn8_u1(const uint8_t vec);

extern inline uint8_t cecs_mark_zero_bytes8_u8(const uint64_t vec);
extern inline uint_fast8_t cecs_first_zero_byte8_u8(const uint64_t vec);
extern inline uint8_t cecs_mark_pattern_bytes8_u8(const uint64_t vec, const uint8_t pattern);
extern inline uint_fast8_t cecs_first_pattern_byte8_u8(const uint64_t vec, const uint8_t pattern);

extern inline uint8_t cecs_mark_zero_nibbles8_u4(const uint32_t vec);
extern inline uint_fast8_t cecs_first_zero_nibble8_u4(const uint32_t vec);
extern inline uint_fast8_t cecs_mark_pattern_nibbles8_u4(const uint32_t vec, const uint8_t pattern);
extern inline uint_fast8_t cecs_first_pattern_nibble8_u4(const uint32_t vec, const uint8_t pattern);