#include "cecs_error.h"

cecs_noreturn extern inline void cecs_unreachable(void);
cecs_noreturn extern inline void cecs_debugbreak_unreachable(const char *const message);
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
cecs_noreturn extern inline void cecs_debugbreak_unreachable_vformat(const char *const format, va_list args);
#endif
cecs_noreturn extern inline void cecs_debugbreak_unreachable_format(const char *const format, ...);

extern inline bool cecs_expect(const bool condition);
extern inline bool cecs_expect_not(const bool condition);
 
extern inline bool cecs_expect_never(const bool condition);
extern inline bool cecs_expect_always(const bool condition);
 
cecs_noreturn extern inline void cecs_exit_success(void);
cecs_noreturn extern inline void cecs_exit_failure(void);
cecs_noreturn extern inline void cecs_debugbreak_fail(void);
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
cecs_noreturn extern inline void cecs_debugbreak_fail_vformat(const char *const format, va_list args);
#endif
cecs_noreturn extern inline void cecs_debugbreak_fail_format(const char *const format, ...);

cecs_noreturn extern inline void cecs_debugbreak_fail_message(const char *const message);

extern inline void cecs_debugbreak_fail_if(const bool condition, const char *const message);
extern inline void cecs_debugbreak_fail_unless(const bool condition, const char *const message);
extern inline void cecs_debugbreak_fail_if_format(const bool condition, const char *const format, ...);
extern inline void cecs_debugbreak_fail_unless_format(const bool condition, const char *const format, ...);

cecs_noreturn extern inline void cecs_unimplemented_fail(const char *const message);
#if !NDEBUG || CECS_ERROR_PRINTS_STDERR
cecs_noreturn extern inline void cecs_unimplemented_fail_vformat(const char *const format, va_list args);
#endif
cecs_noreturn extern inline void cecs_unimplemented_fail_format(const char *const format, ...);
