#include "dckf_time.h"
#include <cecs_compiler.h>

#if CECS_COMPILER_MASK & CECS_COMPILER_MASK_GCC
#include <time.h>
#elif CECS_COMPILER_MASK & CECS_COMPILER_MASK_MSVC
// #include <synchapi.h>
#include <windows.h>
#else
#error "unsupported platform"
#endif

int dckf_nanosleep(const struct timespec *duration, struct timespec *rem) {
#if CECS_COMPILER_MASK & CECS_COMPILER_MASK_GCC
    return nanosleep(duration, rem);
#elif CECS_COMPILER_MASK & CECS_COMPILER_MASK_MSVC
    (void)rem;
    Sleep((DWORD)(duration->tv_sec * 1000 + duration->tv_nsec / 1000000));
    return 0;
#else
#error "unsupported platform"
#endif
}
