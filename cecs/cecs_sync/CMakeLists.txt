cmake_minimum_required(VERSION 3.30)

project(
    cecs_sync
    VERSION 0.1.0
    DESCRIPTION "Synchronization primitives for CECS"
    LANGUAGES C
)
    include(FetchContent)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
    FetchContent_Declare(
        cecs_meta
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_meta
    )
    FetchContent_Declare(
        cecs_compile_options
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_compile_options
    )
    FetchContent_Declare(
        cecs_error
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_error
    )
    FetchContent_Declare(
        cecs_math
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_math
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_meta cecs_error cecs_math)
    add_library(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE cecs_compile_options cecs_meta cecs_error cecs_math)
    if (MSVC)
        enable_c_compiler_flag_if_supported(FLAG "/experimental:c11atomics" TARGET ${PROJECT_NAME} VISIBILITY PUBLIC REQUIRED)
    endif()

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/cecs_rwlock.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/cecs_rwlock.h
    )