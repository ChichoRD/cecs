# source: https://github.com/saxbophone/CPP20-Cross-Platform-Template/blob/6991098e7f243ae9c6477c601953dd132f499e53/CMakeLists.txt
cmake_minimum_required(VERSION 3.30)

project(cecs_compile_options)
    add_library(${PROJECT_NAME} INTERFACE)
    # TODO: lto in release
    # target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
    

    include(CheckCCompilerFlag)
    include(CMakeParseArguments)
    function(enable_c_compiler_flag_if_supported)
        set(options REQUIRED)
        set(oneValueArgs FLAG TARGET VISIBILITY)
        set(multiValueArgs)
        cmake_parse_arguments(
            ARG_ENABLE_FLAG
            "${options}" "${oneValueArgs}" "${multiValueArgs}"
            ${ARGN}
        )
        

        if (NOT ARG_ENABLE_FLAG_FLAG)
            message(FATAL_ERROR "FLAG argument is required for enable_c_compiler_flag_if_supported")
        endif()
        set(flag "${ARG_ENABLE_FLAG_FLAG}")
        set(target "${PROJECT_NAME}")
        set(visibility PRIVATE)
        set(required "${ARG_ENABLE_FLAG_REQUIRED}")

        if (ARG_ENABLE_FLAG_TARGET)
            set(target "${ARG_ENABLE_FLAG_TARGET}")
        endif()
        if (ARG_ENABLE_FLAG_VISIBILITY)
            set(visibility "${ARG_ENABLE_FLAG_VISIBILITY}")
        endif()


        message(STATUS "[${PROJECT_NAME}] Checking if compiler supports flag '${flag}'")
        check_c_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            message(STATUS "[${PROJECT_NAME}] Enabling flag '${flag}'")
            target_compile_options(${target} ${visibility} "${flag}")
        elseif(required)
            message(FATAL_ERROR "[${PROJECT_NAME}] Required flag '${flag}' is not supported by the compiler")
        else()
            message(WARNING "[${PROJECT_NAME}] Compiler does not support flag '${flag}'")
        endif()
        unset(flag_supported CACHE)
    endfunction()


    if (MSVC)
        enable_c_compiler_flag_if_supported(FLAG "/W4" VISIBILITY INTERFACE)
        enable_c_compiler_flag_if_supported(FLAG "/WX" VISIBILITY INTERFACE)
    else()
        enable_c_compiler_flag_if_supported(FLAG "-Wall" VISIBILITY INTERFACE)
        enable_c_compiler_flag_if_supported(FLAG "-Werror" VISIBILITY INTERFACE)
        enable_c_compiler_flag_if_supported(FLAG "-Wextra" VISIBILITY INTERFACE)
        enable_c_compiler_flag_if_supported(FLAG "-Wpedantic" VISIBILITY INTERFACE)
    endif()
    

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
