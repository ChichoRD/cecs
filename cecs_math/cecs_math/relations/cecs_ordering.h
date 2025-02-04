#ifndef CECS_ORDERING_H
#define CECS_ORDERING_H

#include <stdint.h>

inline uint32_t cecs_max_u32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

inline uint32_t cecs_min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}


inline size_t cecs_max(size_t a, size_t b) {
    return a > b ? a : b;
}

inline size_t cecs_min(size_t a, size_t b) {
    return a < b ? a : b;
}

#endif