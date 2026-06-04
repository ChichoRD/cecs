cmake_minimum_required(VERSION 3.30)

project(
    deckfencer
    VERSION 0.1.0
    DESCRIPTION "A sample console game using CECS"
    LANGUAGES C
)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
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

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/main.c
            src/dckf_input.c
            src/dckf_time.c
        
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/dckf_input.h
                src/dckf_time.h
    )
