#ifndef CECS_ATTRIBUTE_H
#define CECS_ATTRIBUTE_H

#include "cecs_compiler.h"


#ifndef cecs_noreturn
#if __STDC_VERSION__ >= 202311L
#define cecs_noreturn [[noreturn]]
#elif __STDC_VERSION__ >= 201112L
#define cecs_noreturn _Noreturn
#else
#define cecs_noreturn 
#pragma message ( "[cecs_meta] warning: noreturn attribute is not supported by this compiler, 'cecs_noreturn' will be defined as empty" )
#endif
#endif


#endif
