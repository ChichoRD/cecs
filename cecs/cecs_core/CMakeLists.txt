cmake_minimum_required(VERSION 3.30)

project(
    cecs_core
    VERSION 0.1.0
    DESCRIPTION "Standard library for CECS"
    LANGUAGES C
)
    option(CECS_CORE_BUILD_EXTENSION "Build the CECS core extension library (cecs_extension)" OFF)

    include(FetchContent)
    FetchContent_Declare(
        cecs_world
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cecs_world
    )
    FetchContent_MakeAvailable(cecs_world)

    if (${CECS_CORE_BUILD_EXTENSION})
        FetchContent_Declare(
            cecs_extension
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cecs_extension
        )
        FetchContent_MakeAvailable(cecs_extension)
    endif()

    
    add_library(${PROJECT_NAME} INTERFACE)
    add_library(cecs ALIAS ${PROJECT_NAME})
    
    target_link_libraries(${PROJECT_NAME} INTERFACE cecs_world)
    if (${CECS_CORE_BUILD_EXTENSION})
        target_link_libraries(${PROJECT_NAME} INTERFACE cecs_extension)
    endif()