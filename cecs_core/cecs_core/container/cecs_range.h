#ifndef CECS_RANGE_H
#define CECS_RANGE_H

#include <stdint.h>
#include <stdbool.h>
#include <cecs_math/relations/cecs_ordering.h>

typedef struct cecs_range {
    size_t start;
    size_t end;
} cecs_range;

static inline bool cecs_range_is_left_subrange(const cecs_range subrange, const cecs_range superrange) {
    return subrange.start >= superrange.start;
}
static inline bool cecs_range_is_right_subrange(const cecs_range subrange, const cecs_range superrange) {
    return subrange.end <= superrange.end;
}
static inline bool cecs_range_is_subrange(const cecs_range subrange, const cecs_range superrange) {
    return cecs_range_is_right_subrange(subrange, superrange) && cecs_range_is_left_subrange(subrange, superrange);
}

static inline bool cecs_range_left_equals(const cecs_range range1, const cecs_range range2) {
    return range1.start == range2.start;
}
static inline bool cecs_range_right_equals(const cecs_range range1, const cecs_range range2) {
    return range1.end == range2.end;
}
static inline bool cecs_range_equals(const cecs_range range1, const cecs_range range2) {
    return cecs_range_left_equals(range1, range2) && cecs_range_right_equals(range1, range2);
}
    
static inline cecs_range cecs_range_intersection(const cecs_range range1, const cecs_range range2) {
    return (cecs_range) {
        .start = cecs_max(range1.start, range2.start),
        .end = cecs_min(range1.end, range2.end)
    };
}
static inline cecs_range cecs_range_union(const cecs_range range1, const cecs_range range2) {
    return (cecs_range) {
        .start = cecs_min(range1.start, range2.start),
        .end = cecs_max(range1.end, range2.end)
    };
}


typedef struct cecs_exclusive_range {
    cecs_range range;
} cecs_exclusive_range;

static inline cecs_exclusive_range cecs_exclusive_range_from(const cecs_range range) {
    return (cecs_exclusive_range) { .range = range };
}
static inline cecs_exclusive_range cecs_exclusive_range_from_inclusive(const cecs_range range) {
    return (cecs_exclusive_range) { .range = { .start = range.start, .end = range.end + 1 } };
}
static inline cecs_exclusive_range cecs_exclusive_range_singleton(size_t index) {
    return (cecs_exclusive_range) { .range = { .start = index, .end = index + 1 } };
}
static inline cecs_exclusive_range cecs_exclusive_range_index_count(size_t index, size_t count) {
    return (cecs_exclusive_range) { .range = { .start = index, .end = index + count } };
}

static inline size_t cecs_exclusive_range_length(const cecs_exclusive_range range) {
    return range.range.end - range.range.start;
}

static inline bool cecs_exclusive_range_contains(const cecs_exclusive_range range, const size_t index) {
    return range.range.start <= index && index < range.range.end;
}
static inline bool cecs_exclusive_range_is_empty(const cecs_exclusive_range range) {
    return range.range.start >= range.range.end;
}

#endif
