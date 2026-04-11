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
        cecs_lib
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_lib
    )
    FetchContent_Declare(
        cecs_core
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_core
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_lib cecs_core)
    add_executable(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE cecs_compile_options cecs_lib cecs_core)
    enable_c_compiler_flag_if_supported(FLAG "-mlzcnt" TARGET ${PROJECT_NAME} REQUIRED)
    enable_c_compiler_flag_if_supported(FLAG "-mavx" TARGET ${PROJECT_NAME} REQUIRED)
    enable_c_compiler_flag_if_supported(FLAG "-mavx2" TARGET ${PROJECT_NAME} REQUIRED)

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/main.c
    )
