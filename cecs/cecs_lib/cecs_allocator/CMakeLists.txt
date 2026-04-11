cmake_minimum_required(VERSION 3.30)

project(
    cecs_allocator
    VERSION 0.1.0
    DESCRIPTION "Memory allocator library for CECS"
    LANGUAGES C
)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
    include(FetchContent)
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
    FetchContent_Declare(
        cecs_memory
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_memory
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_meta cecs_error cecs_math cecs_memory)
    add_library(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE cecs_compile_options cecs_meta PUBLIC cecs_error PRIVATE cecs_math cecs_memory)

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/cecs_allocation.c
            src/cecs_allocator.c
            src/cecs_arena_allocator.c
            src/cecs_bump_allocator.c
            src/cecs_implicit_arena_allocator.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/cecs_allocation.h
                src/cecs_allocator.h
                src/cecs_arena_allocator.h
                src/cecs_bump_allocator.h
                src/cecs_implicit_arena_allocator.h
    )