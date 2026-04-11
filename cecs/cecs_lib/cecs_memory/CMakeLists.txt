cmake_minimum_required(VERSION 3.30)

project(
    cecs_memory
    VERSION 0.1.0
    DESCRIPTION "Memory management library for CECS"
    LANGUAGES C
)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
    include(FetchContent)
    FetchContent_Declare(
        cecs_meta
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_meta
    )
    FetchContent_Declare(
        cecs_error
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_error
    )
    FetchContent_Declare(
        cecs_compile_options
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_compile_options
    )
    FetchContent_Declare(
        cecs_math
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_math
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_meta cecs_math cecs_error)
    add_library(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE cecs_compile_options cecs_meta cecs_math cecs_error)

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/cecs_memory.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/cecs_memory.h
    )