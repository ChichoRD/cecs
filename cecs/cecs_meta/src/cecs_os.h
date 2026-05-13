#ifndef CECS_OS_H
#define CECS_OS_H


#ifndef CECS_OS_MASK
#ifdef CECS_OS
#error "CECS_OS_MASK is not defined, but CECS_OS is defined. Please define CECS_OS_MASK to the appropriate value for your OS."
#endif


#ifndef CECS_OS_MASK_NONE
#define CECS_OS_MASK_NONE 0x00
#endif
#ifndef CECS_OS_MASK_WIN32
#define CECS_OS_MASK_WIN32 0x01
#endif
#ifndef CECS_OS_MASK_WIN64
#define CECS_OS_MASK_WIN64 0x02
#endif
#ifndef CECS_OS_MASK_CYGWIN
#define CECS_OS_MASK_CYGWIN 0x04
#endif
#ifndef CECS_OS_MASK_UNIX
#define CECS_OS_MASK_UNIX 0x08
#endif
#ifndef CECS_OS_MASK_MACOS
#define CECS_OS_MASK_MACOS 0x10
#endif
#ifndef CECS_OS_MASK_LINUX
#define CECS_OS_MASK_LINUX 0x20
#endif
#ifndef CECS_OS_MASK_FREEBSD
#define CECS_OS_MASK_FREEBSD 0x40
#endif
#ifndef CECS_OS_MASK_ANDROID
#define CECS_OS_MASK_ANDROID 0x80
#endif


#ifndef CECS_OS_WIN32
#ifdef _WIN32
#define CECS_OS_WIN32 CECS_OS_MASK_WIN32
#else
#define CECS_OS_WIN32 CECS_OS_MASK_NONE
#endif
#endif
#ifndef CECS_OS_WIN64
#ifdef _WIN64
#define CECS_OS_WIN64 CECS_OS_MASK_WIN64
#else
#define CECS_OS_WIN64 CECS_OS_MASK_NONE
#endif
#endif
#ifndef CECS_OS_CYGWIN
#ifdef __CYGWIN__
#define CECS_OS_CYGWIN CECS_OS_MASK_CYGWIN
#else
#define CECS_OS_CYGWIN CECS_OS_MASK_NONE
#endif
#endif
#ifndef CECS_OS_UNIX
#ifdef __unix__
#define CECS_OS_UNIX CECS_OS_MASK_UNIX
#else
#define CECS_OS_UNIX CECS_OS_MASK_NONE
#endif
#endif
#ifndef CECS_OS_MACOS
#if defined(__APPLE__) && defined(__MACH__)
#define CECS_OS_MACOS CECS_OS_MASK_MACOS
#else
#define CECS_OS_MACOS CECS_OS_MASK_NONE
#endif
#endif
#ifndef CECS_OS_LINUX
#ifdef __linux__
#define CECS_OS_LINUX CECS_OS_MASK_LINUX
#else
#define CECS_OS_LINUX CECS_OS_MASK_NONE
#endif
#endif
#ifndef CECS_OS_FREEBSD
#ifdef __FreeBSD__
#define CECS_OS_FREEBSD CECS_OS_MASK_FREEBSD
#else
#define CECS_OS_FREEBSD CECS_OS_MASK_NONE
#endif
#endif
#ifndef CECS_OS_ANDROID
#ifdef __ANDROID__
#define CECS_OS_ANDROID CECS_OS_MASK_ANDROID
#else
#define CECS_OS_ANDROID CECS_OS_MASK_NONE
#endif
#endif


#define CECS_OS_MASK ( \
    CECS_OS_WIN32 \
    | CECS_OS_WIN64 \
    | CECS_OS_CYGWIN \
    | CECS_OS_UNIX \
    | CECS_OS_MACOS \
    | CECS_OS_LINUX \
    | CECS_OS_FREEBSD \
    | CECS_OS_ANDROID \
)
#if CECS_OS_WIN32 == CECS_OS_MASK_WIN32
#define CECS_OS CECS_OS_MASK_WIN32
#elif CECS_OS_WIN64 == CECS_OS_MASK_WIN64
#define CECS_OS CECS_OS_MASK_WIN64
#elif CECS_OS_CYGWIN == CECS_OS_MASK_CYGWIN
#define CECS_OS CECS_OS_MASK_CYGWIN
#elif CECS_OS_UNIX == CECS_OS_MASK_UNIX
#define CECS_OS CECS_OS_MASK_UNIX
#elif CECS_OS_MACOS == CECS_OS_MASK_MACOS
#define CECS_OS CECS_OS_MASK_MACOS
#elif CECS_OS_LINUX == CECS_OS_MASK_LINUX
#define CECS_OS CECS_OS_MASK_LINUX
#elif CECS_OS_FREEBSD == CECS_OS_MASK_FREEBSD
#define CECS_OS CECS_OS_MASK_FREEBSD
#elif CECS_OS_ANDROID == CECS_OS_MASK_ANDROID
#define CECS_OS CECS_OS_MASK_ANDROID
#else
#define CECS_OS CECS_OS_MASK_NONE
#endif


#if CECS_OS == CECS_OS_MASK_NONE
#error "unsupported os, currently supported oss are: win32, win64, cygwin, unix, macos, linux, freebsd, android"
#endif


#ifndef CECS_OS_MASK_WINDOWS
#define CECS_OS_MASK_WINDOWS (CECS_OS_MASK_WIN32 | CECS_OS_MASK_WIN64 | CECS_OS_MASK_CYGWIN)
#endif
#ifndef CECS_OS_MASK_POSIX
#define CECS_OS_MASK_POSIX (CECS_OS_MASK_UNIX | CECS_OS_MASK_MACOS | CECS_OS_MASK_LINUX | CECS_OS_MASK_FREEBSD | CECS_OS_MASK_ANDROID)
#endif

#endif


#endif
