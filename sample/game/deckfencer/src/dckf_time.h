#ifndef DCKF_TIME_H
#define DCKF_TIME_H

#include <time.h>

int dckf_nanosleep(const struct timespec *duration, struct timespec *rem);

#endif
