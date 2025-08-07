#ifndef CECS_UNITS_H
#define CECS_UNITS_H

#include <assert.h>
#include <stdint.h>

static_assert(sizeof(float) == 4, "static error: float type must be 4 bytes");
static_assert(sizeof(double) == 8, "static error: double type must be 8 bytes");

typedef float cecs_seconds_f32;
typedef double cecs_seconds_f64;

typedef float cecs_radians_f32;
typedef double cecs_radians_f64;

#endif
