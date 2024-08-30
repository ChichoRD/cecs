#ifndef RANGE_H
#define RANGE_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

typedef ptrdiff_t ssize_t;

inline ssize_t ssize_t_max(ssize_t a, ssize_t b) {
    return a > b ? a : b;
}

inline ssize_t ssize_t_min(ssize_t a, ssize_t b) {
    return a < b ? a : b;
}

typedef struct range {
    ssize_t start;
    ssize_t end;
} range;

inline bool range_is_left_subrange(const range superrange, const range subrange) {
    return subrange.start >= superrange.start;
}

inline bool range_is_right_subrange(const range superrange, const range subrange) {
    return subrange.end <= superrange.end;
}

inline bool range_is_subrange(const range superrange, const range subrange) {
    return range_is_right_subrange(superrange, subrange) && range_is_left_subrange(superrange, subrange);
}

inline bool range_leff_equals(const range range1, const range range2) {
    return range1.start == range2.start;
}

inline bool range_right_equals(const range range1, const range range2) {
    return range1.end == range2.end;
}

inline bool range_equals(const range range1, const range range2) {
    return range_leff_equals(range1, range2) && range_right_equals(range1, range2);
}

inline range range_intersection(const range range1, const range range2) {
    return (range) {
        .start = ssize_t_max(range1.start, range2.start),
        .end = ssize_t_min(range1.end, range2.end)
    };
}

inline range range_union(const range range1, const range range2) {
    return (range) {
        .start = ssize_t_min(range1.start, range2.start),
        .end = ssize_t_max(range1.end, range2.end)
    };
}

typedef struct range_splitting {
    range ranges[2];
} range_splitting;


typedef union exclusive_range {
    struct {
        ssize_t start;
        ssize_t end;
    };
    range range;
} exclusive_range;

inline exclusive_range exclusive_range_from(const range range) {
    return (exclusive_range) { .range = range };
}

inline exclusive_range exclusive_range_singleton(ssize_t index) {
    return (exclusive_range) { .start = index, .end = index + 1 };
}

inline ssize_t exclusive_range_length(const exclusive_range range) {
    return range.end - range.start;
}

inline bool exclusive_range_contains(const exclusive_range range, const ssize_t index) {
    return range.start <= index && index < range.end;
}

inline bool exclusive_range_is_empty(const exclusive_range range) {
    return range.start >= range.end;
}

inline range_splitting exclusive_range_difference(const exclusive_range range1, const exclusive_range range2) {
    return (range_splitting) {
        .ranges = {
            (range) { .start = range1.start, .end = range2.start },
            (range) { .start = range2.end, .end = range1.end }
        }
    };
}


typedef union inclusive_range {
    struct {
        ssize_t start;
        ssize_t end;
    };
    range range;
} inclusive_range;

inline inclusive_range inclusive_range_from(const range range) {
    return (inclusive_range) { .range = range };
}

inline inclusive_range inclusive_range_singleton(ssize_t index) {
    return (inclusive_range) { .start = index, .end = index };
}

inline ssize_t range_length(const inclusive_range range) {
    return range.end - range.start + 1;
}

inline bool inclusive_range_contains(const inclusive_range range, const ssize_t index) {
    return range.start <= index && index <= range.end;
}

inline bool inclusive_range_is_empty(const inclusive_range range) {
    return range.start > range.end;
}

inline range_splitting inclusive_range_difference(const inclusive_range range1, const inclusive_range range2) {
    return (range_splitting) {
        .ranges = {
            (range) { .start = range1.start, .end = range2.start - 1 },
            (range) { .start = range2.end + 1, .end = range1.end }
        }
    };
}

#endif