cmake_minimum_required(VERSION 3.30)

project(
    cecs_container
    VERSION 0.1.0
    DESCRIPTION "Container library for CECS"
    LANGUAGES C
)
    include(FetchContent)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
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
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_core/cecs_memory
    )
    FetchContent_Declare(
        cecs_allocator
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_core/cecs_allocator
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_meta cecs_error cecs_math cecs_memory cecs_allocator)
    add_library(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE cecs_compile_options cecs_meta cecs_error PUBLIC cecs_math PRIVATE cecs_memory cecs_allocator)

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/cecs_dynarray.c
            src/cecs_flatbucket.c
            src/cecs_flatmap.c
            src/cecs_flatset.c
            src/cecs_sparse_set.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/cecs_dynarray.h
                src/cecs_flatbucket.h
                src/cecs_flatmap.h
                src/cecs_flatset.h
                src/cecs_range.h
                src/cecs_sparse_set.h
    )