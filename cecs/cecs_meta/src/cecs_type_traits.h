#ifndef CECS_TYPE_TRAITS_H
#define CECS_TYPE_TRAITS_H

#include "cecs_compiler.h"


#ifndef CECS_IS_SAME_TYPE_LR
#ifdef CECS_IS_SAME_TYPE
#error "CECS_IS_SAME_TYPE_LR is not defined, but CECS_IS_SAME_TYPE is defined." \
    "Please define CECS_IS_SAME_TYPE_LR to the appropriate implementation that supports left and right type parameters."
#endif

#if CECS_STDC_VERSION >= CECS_STDC11
#define CECS_IS_SAME_TYPE_LR(type1, type2) _Generic(( *((type1*)0) ), \
    type2: 1, \
    default: 0 \
) 
#define CECS_IS_SAME_TYPE(type1, type2) (CECS_IS_SAME_TYPE_LR(type1, type2) && CECS_IS_SAME_TYPE_LR(type2, type1))
#else

#pragma message ( "[cecs_meta] warning: type trait 'is_same_type' is not supported by this compiler, 'CECS_IS_SAME_TYPE' will be defined as a stub that always returns false" )
#define CECS_IS_SAME_TYPE_LR(type1, type2) (0)
#define CECS_IS_SAME_TYPE(type1, type2) (0)

#endif

#endif


#endif
