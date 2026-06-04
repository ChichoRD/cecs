cmake_minimum_required(VERSION 3.30)

project(
    cecs_meta
    VERSION 0.1.0
    DESCRIPTION "Meta library for CECS"
    LANGUAGES C
)
    include(FetchContent)
    FetchContent_Declare(
        cecs_compile_options
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cecs_compile_options
    )
    FetchContent_MakeAvailable(cecs_compile_options)
    add_library(${PROJECT_NAME} INTERFACE)
    target_link_libraries(${PROJECT_NAME} INTERFACE cecs_compile_options)

    if (MSVC)
        cecs_add_c_compiler_option(OPTION "/Zc:__STDC__" VISIBILITY INTERFACE REQUIRED)
    endif()

    target_sources(
        ${PROJECT_NAME}
        INTERFACE
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/cecs_compiler.h
                src/cecs_os.h
                src/cecs_attribute.h
                src/cecs_console_colorcode.h
                src/cecs_type_traits.h
                src/cecs_platform.h
    )