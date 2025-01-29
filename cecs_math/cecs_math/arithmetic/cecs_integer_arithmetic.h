#ifndef CECS_INTEGER_ARITHMETIC_H
#define CECS_INTEGER_ARITHMETIC_H

#include <stdint.h>
#include <assert.h>

#if (SIZE_MAX == UINT16_MAX)
#define CECS_SIZE_T_BITS_LOG2 4
#define CECS_SIZE_T_BITS_VALUE 16

#elif (SIZE_MAX == UINT32_MAX)
#define CECS_SIZE_T_BITS_LOG2 5
#define CECS_SIZE_T_BITS_VALUE 32

#elif (SIZE_MAX == UINT64_MAX)
#define CECS_SIZE_T_BITS_LOG2 6
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

inline int_fast8_t cecs_log2(size_t n) {
#if (SIZE_MAX == UINT16_MAX)
    return (int_fast8_t)(CECS_SIZE_T_BITS - __lzcnt16((uint16_t)n) - 1);

#elif (SIZE_MAX == UINT32_MAX)
    return (int_fast8_t)(CECS_SIZE_T_BITS - __lzcnt((uint32_t)n) - 1);

#elif (SIZE_MAX == UINT64_MAX)
    return (int_fast8_t)(CECS_SIZE_T_BITS - __lzcnt64((uint64_t)n) - 1);

#else
    #error TBD code CECS_SIZE_T_BITS
    return 0;

#endif
}

#endif