#ifndef CECS_ORDERING_H
#define CECS_ORDERING_H

#include <stdint.h>

inline uint32_t cecs_max_u32(const uint32_t a, const uint32_t b) {
    return a > b ? a : b;
}
inline uint32_t cecs_min_u32(const uint32_t a, const uint32_t b) {
    return a < b ? a : b;
}


inline size_t cecs_max(const size_t a, const size_t b) {
    return a > b ? a : b;
}
inline size_t cecs_min(const size_t a, const size_t b) {
    return a < b ? a : b;
}


inline float cecs_max_f32(const float a, const float b) {
    return a > b ? a : b;
}
inline float cecs_min_f32(const float a, const float b) {
    return a < b ? a : b;
}

#endif