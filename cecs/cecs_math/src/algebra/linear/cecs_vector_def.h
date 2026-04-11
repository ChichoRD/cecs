#ifndef CECS_VECTOR_DEF_H
#define CECS_VECTOR_DEF_H

#include <stdint.h>
#include <assert.h>

static_assert(sizeof(float) == 4, "static error: float type must be 4 bytes");
static_assert(sizeof(double) == 8, "static error: double type must be 8 bytes");

typedef struct cecs_vec2_f32 {
    float x;
    float y;
} cecs_vec2_f32;
typedef struct cecs_vec2_i32 {
    int32_t x;
    int32_t y;
} cecs_vec2_i32;
typedef struct cecs_vec2_u32 {
    uint32_t x;
    uint32_t y;
} cecs_vec2_u32;


typedef struct cecs_vec3_f32 {
    float x;
    float y;
    float z;
} cecs_vec3_f32;
typedef struct cecs_vec3_i32 {
    int32_t x;
    int32_t y;
    int32_t z;
} cecs_vec3_i32;
typedef struct cecs_vec3_u32 {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} cecs_vec3_u32;


typedef struct cecs_vec4_f32 {
    float x;
    float y;
    float z;
    float w;
} cecs_vec4_f32;

#endif
