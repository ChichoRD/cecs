#ifndef CECS_COMPILER_H
#define CECS_COMPILER_H


#ifndef CECS_COMPILER_MASK
#ifdef CECS_COMPILER
#error "CECS_COMPILER_MASK is not defined, but CECS_COMPILER is defined. Please define CECS_COMPILER_MASK to the appropriate value for your compiler."
#endif


#ifndef CECS_COMPILER_MASK_NONE
#define CECS_COMPILER_MASK_NONE 0x00
#endif
#ifndef CECS_COMPILER_MASK_CLANG
#define CECS_COMPILER_MASK_CLANG 0x01
#endif
#ifndef CECS_COMPILER_MASK_GCC
#define CECS_COMPILER_MASK_GCC 0x02
#endif
#ifndef CECS_COMPILER_MASK_MSVC
#define CECS_COMPILER_MASK_MSVC 0x04
#endif


#ifndef cecs_compiler_backend_clang
#ifdef __clang__
#define cecs_compiler_backend_clang CECS_COMPILER_MASK_CLANG
#else
#define cecs_compiler_backend_clang CECS_COMPILER_MASK_NONE
#endif
#endif

#ifndef cecs_compiler_backend_gcc
#if defined(__GNUC__) || defined(__GNUG__)
#define cecs_compiler_backend_gcc CECS_COMPILER_MASK_GCC
#else
#define cecs_compiler_backend_gcc CECS_COMPILER_MASK_NONE
#endif
#endif

#ifndef cecs_compiler_backend_msvc
#ifdef _MSC_VER
#define cecs_compiler_backend_msvc CECS_COMPILER_MASK_MSVC
#else
#define cecs_compiler_backend_msvc CECS_COMPILER_MASK_NONE
#endif
#endif


#define CECS_COMPILER_MASK (cecs_compiler_backend_clang | cecs_compiler_backend_gcc | cecs_compiler_backend_msvc)
#if cecs_compiler_backend_clang == CECS_COMPILER_MASK_CLANG
#define CECS_COMPILER CECS_COMPILER_MASK_CLANG
#elif cecs_compiler_backend_gcc == CECS_COMPILER_MASK_GCC
#define CECS_COMPILER CECS_COMPILER_MASK_GCC
#elif cecs_compiler_backend_msvc == CECS_COMPILER_MASK_MSVC
#define CECS_COMPILER CECS_COMPILER_MASK_MSVC
#else
#define CECS_COMPILER CECS_COMPILER_MASK_NONE
#endif


#if CECS_COMPILER == CECS_COMPILER_MASK_NONE
#error "unsupported compiler, currently supported compilers are: clang, gcc, msvc"
#endif


#undef cecs_compiler_backend_clang
#undef cecs_compiler_backend_gcc
#undef cecs_compiler_backend_msvc


#ifndef CECS_STDC89
#define CECS_STDC89 198900L
#endif
#ifndef CECS_STDC99
#define CECS_STDC99 199901L
#endif
#ifndef CECS_STDC11
#define CECS_STDC11 201112L
#endif
#ifndef CECS_STDC17
#define CECS_STDC17 201710L
#endif
#ifndef CECS_STDC23
#define CECS_STDC23 202311L
#endif

#ifndef CECS_STDC_VERSION
#ifndef __STDC_VERSION__
#pragma message ( "[cecs_meta] warning: __STDC_VERSION__ is not defined, 'CECS_STDC_VERSION' will be defined as CECS_STDC89" )
#define CECS_STDC_VERSION CECS_STDC89
#else
#define CECS_STDC_VERSION __STDC_VERSION__
#endif
#endif


#endif


#endif
