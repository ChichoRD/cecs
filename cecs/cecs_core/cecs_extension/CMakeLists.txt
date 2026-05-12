cmake_minimum_required(VERSION 3.30)

project(
    cecs_extension
    VERSION 0.1.0
    DESCRIPTION "Extension library for CECS, providing additional features and utilities"
    LANGUAGES C
)
    include(FetchContent)
    FetchContent_Declare(
        cecs_world
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cecs_world
    )
    FetchContent_MakeAvailable(cecs_world)
    add_library(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE cecs_world)

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            # src/extension/cecs_extension.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                # src/extension/cecs_extension.h
    )