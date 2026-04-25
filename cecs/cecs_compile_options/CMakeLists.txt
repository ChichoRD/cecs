# source: https://github.com/saxbophone/CPP20-Cross-Platform-Template/blob/6991098e7f243ae9c6477c601953dd132f499e53/CMakeLists.txt
cmake_minimum_required(VERSION 3.30)


function(cecs_add_c_compiler_option)
    include(CheckCCompilerFlag)
    include(CMakeParseArguments)
    set(options REQUIRED)
    set(oneValueArgs OPTION TARGET VISIBILITY)
    set(multiValueArgs)
    cmake_parse_arguments(
        ARG_ADD_OPTION
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
        ${ARGN}
    )
    

    if (NOT ARG_ADD_OPTION_OPTION)
        message(FATAL_ERROR "OPTION argument is required for cecs_add_c_compiler_option")
    endif()
    set(option "${ARG_ADD_OPTION_OPTION}")
    set(target "${PROJECT_NAME}")
    set(visibility PRIVATE)
    set(required "${ARG_ADD_OPTION_REQUIRED}")

    if (ARG_ADD_OPTION_TARGET)
        set(target "${ARG_ADD_OPTION_TARGET}")
    endif()
    if (ARG_ADD_OPTION_VISIBILITY)
        set(visibility "${ARG_ADD_OPTION_VISIBILITY}")
    endif()


    message(STATUS "[${PROJECT_NAME}] Checking if compiler supports option '${option}'")
    check_c_compiler_flag("${option}" option_supported)
    if(option_supported)
        message(STATUS "[${PROJECT_NAME}] Enabling option '${option}'")
        target_compile_options(${target} ${visibility} "${option}")
    elseif(required)
        message(FATAL_ERROR "[${PROJECT_NAME}] Required option '${option}' is not supported by the compiler")
    else()
        message(WARNING "[${PROJECT_NAME}] Compiler does not support option '${option}'")
    endif()
    unset(option_supported CACHE)
endfunction(cecs_add_c_compiler_option)


function(cecs_add_sanitizer_compile_options)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        include(CheckLinkerFlag)

        if (MSVC)
            check_c_compiler_flag("/fsanitize=address" HAS_ASAN_FLAG)
            if (HAS_ASAN_FLAG)
                target_compile_options(${PROJECT_NAME} INTERFACE /fsanitize=address /Zi)
                target_link_options(${PROJECT_NAME} INTERFACE /DEBUG)
            endif()
        else()
            check_linker_flag(CXX "-fsanitize=address" HAS_ASAN_LINK)
            check_c_compiler_flag("-fsanitize=undefined" HAS_UBSAN_FLAG)
            check_linker_flag(CXX "-fsanitize=undefined" HAS_UBSAN_LINK)
            if (HAS_ASAN_LINK AND HAS_UBSAN_FLAG AND HAS_UBSAN_LINK)
                target_compile_options(${PROJECT_NAME} INTERFACE -fsanitize=address,undefined)
                target_link_options(${PROJECT_NAME} INTERFACE -fsanitize=address,undefined)
            endif()
        endif()
    endif()
endfunction(cecs_add_sanitizer_compile_options)


function(cecs_add_default_compile_options)
    include(CMakeParseArguments)
    set(options SANITIZE)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(
        ARG_ADD_DEFAULT_OPTIONS
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
        ${ARGN}
    )
    set(sanitize "${ARG_ADD_DEFAULT_OPTIONS_SANITIZE}")


    if (MSVC)
        cecs_add_c_compiler_option(OPTION "/W4" VISIBILITY INTERFACE)
        cecs_add_c_compiler_option(OPTION "/WX" VISIBILITY INTERFACE)
    else()
        cecs_add_c_compiler_option(OPTION "-Wall" VISIBILITY INTERFACE)
        cecs_add_c_compiler_option(OPTION "-Werror" VISIBILITY INTERFACE)
        cecs_add_c_compiler_option(OPTION "-Wextra" VISIBILITY INTERFACE)
        cecs_add_c_compiler_option(OPTION "-Wpedantic" VISIBILITY INTERFACE)
    endif()

    if (sanitize)
        cecs_add_sanitizer_compile_options()
    endif()
endfunction(cecs_add_default_compile_options)


project(cecs_compile_options)
    add_library(${PROJECT_NAME} INTERFACE)
    # TODO: lto in release
    # target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
    
    cecs_add_default_compile_options(SANITIZE)

    

