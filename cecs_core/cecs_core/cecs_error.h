#ifndef CECS_ERROR_H
#define CECS_ERROR_H

#include <cassert>
#include <cstdlib>

#define CECS_COMPILER_NONE 0
#define CECS_COMPILER_CLANG 1
#define CECS_COMPILER_GCC 2
#define CECS_COMPILER_MSVC 3


#ifdef __clang__
#define CECS_COMPILER CECS_COMPILER_CLANG

#elif defined(__GNUC__) || defined(__GNUG__)
#define CECS_COMPILER CECS_COMPILER_GCC

#elif defined(_MSC_VER)
#define CECS_COMPILER CECS_COMPILER_MSVC

#else
#define CECS_COMPILER CECS_COMPILER_NONE

#endif


#if CECS_COMPILER == CECS_COMPILER_NONE
#error "unsupported compiler"
#endif


inline void cecs_unreachable(void) {
#if CECS_COMPILER == CECS_COMPILER_CLANG
    __builtin_unreachable();
#elif CECS_COMPILER == CECS_COMPILER_GCC
    __builtin_unreachable();
#elif CECS_COMPILER == CECS_COMPILER_MSVC
    __assume(0);
#endif
}

inline void cecs_assert_unreachable(const char *const message) {
    (void)message;
    assert(false && "fatal error: unreachable code reached");
    cecs_unreachable();
}


inline bool cecs_expect(const bool condition) {
#if CECS_COMPILER == CECS_COMPILER_CLANG
    return __builtin_expect((condition), 1);
#elif CECS_COMPILER == CECS_COMPILER_GCC
    return __builtin_expect((condition), 1);
#elif CECS_COMPILER == CECS_COMPILER_MSVC
    if (condition) {
        [[msvc::likely]]
        return true;
    } else {
        return false;
    }
#endif
}
inline bool cecs_expect_not(const bool condition) {
#if CECS_COMPILER == CECS_COMPILER_CLANG
    return __builtin_expect((condition), 0);
#elif CECS_COMPILER == CECS_COMPILER_GCC
    return __builtin_expect((condition), 0);
#elif CECS_COMPILER == CECS_COMPILER_MSVC
    if (condition) {
        [[msvc::unlikely]]
        return true;
    } else {
        return false;
    }
#endif
}

inline bool cecs_expect_never(const bool condition) {
    if (condition) {
        cecs_unreachable();
    }
    return false;
}
inline bool cecs_expect_always(const bool condition) {
    if (!condition) {
        cecs_unreachable();
    }
    return true;
}


inline void cecs_exit_success(void) {
    exit(EXIT_SUCCESS);
}
inline void cecs_exit_failure(void) {
    exit(EXIT_FAILURE);
}

inline void cecs_exit_assert_failure(const char *const message) {
    (void)message;
    assert(false && "fatal error: assert failure and exit called");
    exit(EXIT_FAILURE);
}

inline void cecs_exit_failure_if(const bool condition) {
    if (cecs_expect_not(condition)) {
        cecs_exit_failure();
    }
}
inline void cecs_exit_failure_if_not(const bool condition) {
    if (cecs_expect_not(!condition)) {
        cecs_exit_failure();
    }
}
inline void cecs_exit_assert_failure_if(const bool condition, const char *const message) {
    if (cecs_expect_not(condition)) {
        cecs_exit_assert_failure(message);
    }
}
inline void cecs_exit_assert_failure_if_not(const bool condition, const char *const message) {
    if (cecs_expect_not(!condition)) {
        cecs_exit_assert_failure(message);
    }
}

#undef CECS_COMPILER_NONE
#undef CECS_COMPILER_CLANG
#undef CECS_COMPILER_GCC
#undef CECS_COMPILER_MSVC
#undef CECS_COMPILER

#endif