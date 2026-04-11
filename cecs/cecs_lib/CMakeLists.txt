cmake_minimum_required(VERSION 3.30)

project(
    cecs_lib
    VERSION 0.1.0
    DESCRIPTION "Standard library for CECS"
    LANGUAGES C
)
    include(FetchContent)
    FetchContent_Declare(
        cecs_allocator
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cecs_allocator
    )
    FetchContent_Declare(
        cecs_container
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cecs_container
    )
    FetchContent_Declare(
        cecs_memory
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cecs_memory
    )
    FetchContent_MakeAvailable(cecs_allocator cecs_container cecs_memory)
    add_library(${PROJECT_NAME} INTERFACE)
    target_link_libraries(${PROJECT_NAME} INTERFACE cecs_allocator cecs_container cecs_memory)