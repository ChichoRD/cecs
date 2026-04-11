cmake_minimum_required(VERSION 3.30)

project(
    cecs
    VERSION 0.1.0
    DESCRIPTION "A C Entity Component System library"
    LANGUAGES C
)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    include(FetchContent)
    FetchContent_Declare(
        cecs_core
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_core
    )
    FetchContent_MakeAvailable(cecs_core)


project(
    cecs_samples
    VERSION 0.1.0
    DESCRIPTION "Sample applications for CECS"
    LANGUAGES C
)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    include(FetchContent)
    FetchContent_Declare(
        fiddle_test
        SOURCE_DIR ${CECS_SOURCE_DIR}/sample/fiddle_test
    )
    FetchContent_MakeAvailable(fiddle_test)