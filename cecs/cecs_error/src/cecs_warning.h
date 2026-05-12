#ifndef CECS_WARNING_H
#define CECS_WARNING_H


#ifndef CECS_WARNING_PRINTS_STDERR
#define CECS_WARNING_PRINTS_STDERR_DEFAULT true
#define CECS_WARNING_PRINTS_STDERR CECS_WARNING_PRINTS_STDERR_DEFAULT

#endif


#if !NDEBUG || CECS_WARNING_PRINTS_STDERR
#include <stdio.h>
#include <cecs_console_colorcode.h>

#endif


inline void cecs_warning(const char *const message) {
#if !NDEBUG || CECS_WARNING_PRINTS_STDERR
    fprintf(stderr, CECS_COLORCODE_YELLOW "[cecs] warning:" CECS_COLORCODE_RESET " %s\n", message);
#endif

    (void)message;
}


#endif
