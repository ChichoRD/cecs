cmake_minimum_required(VERSION 3.30)

project(
    fiddle_test
    VERSION 0.1.0
    DESCRIPTION "Test for CECS"
    LANGUAGES C
)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
    include(FetchContent)
    # FIXME: transitivity
    FetchContent_Declare(
        cecs_compile_options
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_compile_options
    )
    FetchContent_Declare(
        cecs_core
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_core
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_core)
    add_executable(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE cecs_compile_options cecs_core)

    if (NOT MSVC)
        cecs_add_c_compiler_option(OPTION "-mlzcnt" TARGET ${PROJECT_NAME} REQUIRED)
        cecs_add_c_compiler_option(OPTION "-mavx" TARGET ${PROJECT_NAME} REQUIRED)
        cecs_add_c_compiler_option(OPTION "-mavx2" TARGET ${PROJECT_NAME} REQUIRED)
    endif()

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/main.c
    )
