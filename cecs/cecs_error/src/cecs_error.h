#ifndef CECS_ERROR_H
#define CECS_ERROR_H


#ifndef CECS_ERROR_PRINTS_STDERR
#define CECS_ERROR_PRINTS_STDERR_DEFAULT true
#define CECS_ERROR_PRINTS_STDERR CECS_ERROR_PRINTS_STDERR_DEFAULT

#endif


#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include <cecs_compiler.h>
#include <cecs_attribute.h>


#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
#include <stdio.h>
#include <cecs_console_colorcode.h>
#endif


inline void cecs_unreachable(void) {
#if CECS_COMPILER == CECS_COMPILER_MASK_CLANG
    __builtin_unreachable();
#elif CECS_COMPILER == CECS_COMPILER_MASK_GCC
    __builtin_unreachable();
#elif CECS_COMPILER == CECS_COMPILER_MASK_MSVC
    __assume(0);
#endif
}

inline void cecs_assert_unreachable(const char *const message) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    fprintf(stderr, CECS_COLORCODE_RED "CECS ERROR!" CECS_COLORCODE_RESET " %s\n", message);
#endif

    (void)message;
    assert(false && "fatal error: unreachable code reached");
    cecs_unreachable();
}


inline bool cecs_expect(const bool condition) {
#if CECS_COMPILER == CECS_COMPILER_MASK_CLANG
    return __builtin_expect((condition), 1);
#elif CECS_COMPILER == CECS_COMPILER_MASK_GCC
    return __builtin_expect((condition), 1);
#elif CECS_COMPILER == CECS_COMPILER_MASK_MSVC
    return condition;
#endif
}
inline bool cecs_expect_not(const bool condition) {
#if CECS_COMPILER == CECS_COMPILER_MASK_CLANG
    return __builtin_expect((condition), 0);
#elif CECS_COMPILER == CECS_COMPILER_MASK_GCC
    return __builtin_expect((condition), 0);
#elif CECS_COMPILER == CECS_COMPILER_MASK_MSVC
    return condition;
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


cecs_noreturn inline void cecs_exit_success(void) {
    exit(EXIT_SUCCESS);
}
cecs_noreturn inline void cecs_exit_failure(void) {
    exit(EXIT_FAILURE);
}
cecs_noreturn inline void cecs_fail(void) {
    assert(false && "fatal error: fail and exit called");
    cecs_exit_failure();
}

cecs_noreturn inline void cecs_assert_and_fail(const char *const message) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    fprintf(stderr, CECS_COLORCODE_RED "CECS ERROR!" CECS_COLORCODE_RESET " %s\n", message);
#endif

    (void)message;
    cecs_fail();
}

inline void cecs_fail_exit_if(const bool condition) {
    if (cecs_expect_not(condition)) {
        cecs_fail();
    }
    cecs_expect_never(condition);
}
inline void cecs_assert_or_exit(const bool condition, const char *const message) {
    if (cecs_expect_not(!condition)) {
        cecs_assert_and_fail(message);
    }
    cecs_expect_always(condition);
}

cecs_noreturn inline void cecs_unimplemented_fail(const char *const message) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    fprintf(stderr, CECS_COLORCODE_YELLOW "CECS UNIMPLEMENTED!" CECS_COLORCODE_RESET " %s\n", message);
#endif

    (void)message;
    cecs_fail();
}

#endif
