#include "cecs_warning.h"

extern inline void cecs_warning(const char *const message);
#if !NDEBUG || CECS_WARNING_PRINTS_STDERR
extern inline void cecs_warning_vformat(const char *const format, va_list args);
#endif
extern inline void cecs_warning_format(const char *const format, ...);
