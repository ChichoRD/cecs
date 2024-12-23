#ifndef CECS_RANGE_H
#define CECS_RANGE_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

typedef ptrdiff_t cecs_ssize_t;

static inline cecs_ssize_t cecs_ssize_t_max(cecs_ssize_t a, cecs_ssize_t b) {
    return a > b ? a : b;
}

static inline cecs_ssize_t cecs_ssize_t_min(cecs_ssize_t a, cecs_ssize_t b) {
    return a < b ? a : b;
}

typedef struct cecs_range {
    cecs_ssize_t start;
    cecs_ssize_t end;
} cecs_range;

static inline bool cecs_range_is_left_subrange(const cecs_range superrange, const cecs_range subrange) {
    return subrange.start >= superrange.start;
}

static inline bool cecs_range_is_right_subrange(const cecs_range superrange, const cecs_range subrange) {
    return subrange.end <= superrange.end;
}

static inline bool cecs_range_is_subrange(const cecs_range superrange, const cecs_range subrange) {
    return cecs_range_is_right_subrange(superrange, subrange) && cecs_range_is_left_subrange(superrange, subrange);
}

static inline bool cecs_range_leff_equals(const cecs_range range1, const cecs_range range2) {
    return range1.start == range2.start;
}

static inline bool cecs_range_right_equals(const cecs_range range1, const cecs_range range2) {
    return range1.end == range2.end;
}

static inline bool cecs_range_equals(const cecs_range range1, const cecs_range range2) {
    return cecs_range_leff_equals(range1, range2) && cecs_range_right_equals(range1, range2);
}

static inline cecs_range cecs_range_intersection(const cecs_range range1, const cecs_range range2) {
    return (cecs_range) {
        .start = cecs_ssize_t_max(range1.start, range2.start),
        .end = cecs_ssize_t_min(range1.end, range2.end)
    };
}

static inline cecs_range cecs_range_union(const cecs_range range1, const cecs_range range2) {
    return (cecs_range) {
        .start = cecs_ssize_t_min(range1.start, range2.start),
        .end = cecs_ssize_t_max(range1.end, range2.end)
    };
}

typedef struct cecs_range_splitting {
    cecs_range ranges[2];
} cecs_range_splitting;


typedef union cecs_exclusive_range {
    struct {
        cecs_ssize_t start;
        cecs_ssize_t end;
    };
    cecs_range range;
} cecs_exclusive_range;

static inline cecs_exclusive_range cecs_exclusive_range_from(const cecs_range range) {
    return (cecs_exclusive_range) { .range = range };
}

static inline cecs_exclusive_range cecs_exclusive_range_from_inclusive(const cecs_range range) {
    return (cecs_exclusive_range) { .start = range.start, .end = range.end + 1 };
}

static inline cecs_exclusive_range cecs_exclusive_range_singleton(cecs_ssize_t index) {
    return (cecs_exclusive_range) { .start = index, .end = index + 1 };
}

static inline cecs_exclusive_range cecs_exclusive_range_index_count(cecs_ssize_t index, cecs_ssize_t count) {
    return (cecs_exclusive_range) { .start = index, .end = index + count };
}

static inline cecs_ssize_t cecs_exclusive_range_length(const cecs_exclusive_range range) {
    return range.end - range.start;
}

static inline bool cecs_exclusive_range_contains(const cecs_exclusive_range range, const cecs_ssize_t index) {
    return range.start <= index && index < range.end;
}

static inline bool cecs_exclusive_range_is_empty(const cecs_exclusive_range range) {
    return range.start >= range.end;
}

static inline cecs_range_splitting cecs_exclusive_range_difference(const cecs_exclusive_range range1, const cecs_exclusive_range range2) {
    return (cecs_range_splitting) {
        .ranges = {
            (cecs_range) { .start = range1.start, .end = range2.start },
            (cecs_range) { .start = range2.end, .end = range1.end }
        }
    };
}


typedef union cecs_inclusive_range {
    struct {
        cecs_ssize_t start;
        cecs_ssize_t end;
    };
    cecs_range range;
} cecs_inclusive_range;

static inline cecs_inclusive_range cecs_inclusive_range_from(const cecs_range range) {
    return (cecs_inclusive_range) { .range = range };
}

static inline cecs_inclusive_range cecs_inclusive_range_from_exclusive(const cecs_range range) {
    return (cecs_inclusive_range) { .start = range.start, .end = range.end - 1 };
}

static inline cecs_inclusive_range cecs_inclusive_range_singleton(cecs_ssize_t index) {
    return (cecs_inclusive_range) { .start = index, .end = index };
}

static inline cecs_inclusive_range cecs_inclusive_range_index_count(cecs_ssize_t index, cecs_ssize_t count) {
    return (cecs_inclusive_range) { .start = index, .end = index + count - 1 };
}

static inline cecs_ssize_t cecs_inclusive_range_length(const cecs_inclusive_range range) {
    return range.end - range.start + 1;
}

static inline bool cecs_inclusive_range_contains(const cecs_inclusive_range range, const cecs_ssize_t index) {
    return range.start <= index && index <= range.end;
}

static inline bool cecs_inclusive_range_is_empty(const cecs_inclusive_range range) {
    return range.start > range.end;
}

static inline cecs_range_splitting cecs_inclusive_range_difference(const cecs_inclusive_range range1, const cecs_inclusive_range range2) {
    return (cecs_range_splitting) {
        .ranges = {
            (cecs_range) { .start = range1.start, .end = range2.start - 1 },
            (cecs_range) { .start = range2.end + 1, .end = range1.end }
        }
    };
}

#endif