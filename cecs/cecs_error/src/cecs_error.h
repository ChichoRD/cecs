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
#include <stdarg.h>
#include <cecs_console_colorcode.h>
#endif


cecs_noreturn inline void cecs_unreachable(void) {
#if CECS_COMPILER == CECS_COMPILER_MASK_CLANG
    __builtin_unreachable();
#elif CECS_COMPILER == CECS_COMPILER_MASK_GCC
    __builtin_unreachable();
#elif CECS_COMPILER == CECS_COMPILER_MASK_MSVC
    __assume(0);
#endif
}
cecs_noreturn inline void cecs_debugbreak_unreachable(const char *const message) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    fprintf(stderr, CECS_COLORCODE_RED "[cecs] error:" CECS_COLORCODE_RESET " %s\n", message);
#endif

    (void)message;
    assert(false && "[cecs] fatal error: unreachable code reached");
    cecs_unreachable();
}
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
cecs_noreturn inline void cecs_debugbreak_unreachable_vformat(const char *const format, va_list args) {
    fprintf(stderr, CECS_COLORCODE_RED "[cecs] error:" CECS_COLORCODE_RESET " ");
    vfprintf(stderr, format, args);
    
    (void)format;
    assert(false && "[cecs] fatal error: unreachable code reached");
    cecs_unreachable();
}
#endif
cecs_noreturn inline void cecs_debugbreak_unreachable_format(const char *const format, ...) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    va_list args;
    va_start(args, format);
    cecs_debugbreak_unreachable_vformat(format, args);
    va_end(args);
#else
    (void)format;
    assert(false && "[cecs] fatal error: unreachable code reached");
    cecs_unreachable();
#endif
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
cecs_noreturn inline void cecs_debugbreak_fail(void) {
    assert(false && "[cecs] fatal error: fail and exit called");
    cecs_exit_failure();
}

cecs_noreturn inline void cecs_debugbreak_fail_message(const char *const message) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    fprintf(stderr, CECS_COLORCODE_RED "[cecs] error:" CECS_COLORCODE_RESET " %s\n", message);
#endif

    (void)message;
    cecs_debugbreak_fail();
}
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
cecs_noreturn inline void cecs_debugbreak_fail_vformat(const char *const format, va_list args) {
    fprintf(stderr, CECS_COLORCODE_RED "[cecs] error:" CECS_COLORCODE_RESET " ");    
    vfprintf(stderr, format, args);
    
    (void)format;
    cecs_debugbreak_fail();
}
#endif
cecs_noreturn inline void cecs_debugbreak_fail_format(const char *const format, ...) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    va_list args;
    va_start(args, format);
    cecs_debugbreak_fail_vformat(format, args);
    va_end(args);
#else
    (void)format;
    cecs_debugbreak_fail();
#endif
}


inline void cecs_debugbreak_fail_if(const bool condition, const char *const message) {
    if (cecs_expect_not(condition)) {
        cecs_debugbreak_fail_message(message);
    }
    cecs_expect_never(condition);
}
inline void cecs_debugbreak_fail_unless(const bool condition, const char *const message) {
    if (cecs_expect_not(!condition)) {
        cecs_debugbreak_fail_message(message);
    }
    cecs_expect_always(condition);
}

inline void cecs_debugbreak_fail_if_format(const bool condition, const char *const format, ...) {
    if (cecs_expect_not(condition)) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
        va_list args;
        va_start(args, format);
        cecs_debugbreak_fail_vformat(format, args);
        va_end(args);
#endif
    }
    cecs_expect_never(condition);
}
inline void cecs_debugbreak_fail_unless_format(const bool condition, const char *const format, ...) {
    if (cecs_expect_not(!condition)) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
        va_list args;
        va_start(args, format);
        cecs_debugbreak_fail_vformat(format, args);
        va_end(args);
#endif
    }
    cecs_expect_always(condition);
}

cecs_noreturn inline void cecs_unimplemented_fail(const char *const message) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    fprintf(stderr, CECS_COLORCODE_YELLOW "[cecs] unimplemented error:" CECS_COLORCODE_RESET " %s\n", message);
#endif

    (void)message;
    cecs_debugbreak_fail();
}
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
cecs_noreturn inline void cecs_unimplemented_fail_vformat(const char *const format, va_list args) {
    fprintf(stderr, CECS_COLORCODE_YELLOW "[cecs] unimplemented error:" CECS_COLORCODE_RESET " ");
    vfprintf(stderr, format, args);
    
    (void)format;
    cecs_debugbreak_fail();
}
#endif
cecs_noreturn inline void cecs_unimplemented_fail_format(const char *const format, ...) {
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
    va_list args;
    va_start(args, format);
    cecs_unimplemented_fail_vformat(format, args);
    va_end(args);
#else
    (void)format;
    cecs_debugbreak_fail();
#endif
}
#endif
