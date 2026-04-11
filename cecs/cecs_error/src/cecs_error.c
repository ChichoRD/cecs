#include "cecs_error.h"

extern inline void cecs_unreachable(void);
extern inline void cecs_debugbreak_unreachable(const char *const message);

extern inline bool cecs_expect(const bool condition);
extern inline bool cecs_expect_not(const bool condition);
 
extern inline bool cecs_expect_never(const bool condition);
extern inline bool cecs_expect_always(const bool condition);
 
cecs_noreturn extern inline void cecs_exit_success(void);
cecs_noreturn extern inline void cecs_exit_failure(void);
cecs_noreturn extern inline void cecs_debugbreak_fail(void);

cecs_noreturn extern inline void cecs_debugbreak_fail_message(const char *const message);

extern inline void cecs_debugbreak_fail_if(const bool condition, const char *const message);
extern inline void cecs_debugbreak_fail_unless(const bool condition, const char *const message);

cecs_noreturn extern inline void cecs_unimplemented_fail(const char *const message);
