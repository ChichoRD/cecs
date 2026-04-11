cmake_minimum_required(VERSION 3.30)

project(
    cecs_error
    VERSION 0.1.0
    DESCRIPTION "Error handling library for CECS"
    LANGUAGES C
)
    include(FetchContent)
    FetchContent_Declare(
        cecs_meta
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cecs_meta
    )
    FetchContent_Declare(
        cecs_compile_options
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cecs_compile_options
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_meta)
    add_library(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PUBLIC cecs_compile_options cecs_meta)

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/cecs_error.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/cecs_error.h
    )