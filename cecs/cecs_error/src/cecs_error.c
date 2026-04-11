#include "cecs_error.h"

extern inline void cecs_unreachable(void);
extern inline void cecs_assert_unreachable(const char *const message);

extern inline bool cecs_expect(const bool condition);
extern inline bool cecs_expect_not(const bool condition);
 
extern inline bool cecs_expect_never(const bool condition);
extern inline bool cecs_expect_always(const bool condition);
 
cecs_noreturn extern inline void cecs_exit_success(void);
cecs_noreturn extern inline void cecs_exit_failure(void);
cecs_noreturn extern inline void cecs_fail(void);

cecs_noreturn extern inline void cecs_assert_and_fail(const char *const message);

extern inline void cecs_fail_exit_if(const bool condition);
extern inline void cecs_assert_or_exit(const bool condition, const char *const message);

cecs_noreturn extern inline void cecs_unimplemented_fail(const char *const message);
