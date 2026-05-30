cmake_minimum_required(VERSION 3.30)

project(
    cecs_query
    VERSION 0.1.0
    DESCRIPTION "Extension library for CECS, providing additional features and utilities related to querying entities and components"
    LANGUAGES C
)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
    include(FetchContent)
    FetchContent_Declare(
        cecs_world
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_world
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
    FetchContent_MakeAvailable(cecs_compile_options cecs_error cecs_math cecs_world)
    add_library(${PROJECT_NAME} INTERFACE)
    target_link_libraries(${PROJECT_NAME} INTERFACE cecs_compile_options cecs_error cecs_math cecs_world)

    target_sources(
        ${PROJECT_NAME}
        INTERFACE
            src/query/cecs_query_descriptor.c
            src/cecs_query.c
            src/cecs_iter.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS 
                src
            FILES
                src/query/cecs_query_descriptor.h
                src/cecs_query.h
                src/cecs_iter.h
    )