cmake_minimum_required(VERSION 3.15...3.30)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

project(cecs_math C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)

    if (MSVC)
        message(STATUS "MSVC detected")
        add_compile_options(
            /Zc:preprocessor
            /we\"4022\"

            /W4
            /wd\"5045\"
            /wd\"4115\"
            /wd\"4255\"
            /wd\"4668\"

            /Zc:wchar_t
            /Zi

            # /EHsc
            /nologo
        )
    else()
        message(STATUS "GCC/Clang detected")
        add_compile_options(
            -Wall
            -Wextra
            -Wpedantic

            -Wno-gnu-zero-variadic-macro-arguments
            -Wno-newline-eof
            -Wno-unused-function
            -Wno-extra-semi
            -Werror

            -mlzcnt
        )
    endif()

    set(
        CECS_MATH_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/cecs_math/"
    )
    set(
        CECS_MATH_SRC_DIRECTORY
        "${CECS_MATH_DIRECTORY}/cecs_math/"
    )

    file(
        GLOB_RECURSE
        CECS_MATH_SOURCES
        CMAKE_CONFIGURE_DEPENDS
        ${CECS_MATH_SRC_DIRECTORY}/*.c
    )
    add_library(
        ${PROJECT_NAME}
        STATIC
        ${CECS_MATH_SOURCES}
    )

    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CECS_MATH_DIRECTORY}
    )

project(cecs_app C)
    set(
        CECS_CORE_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/cecs_core/"
    )
    set(
        CECS_CORE_SRC_DIRECTORY
        "${CECS_CORE_DIRECTORY}/cecs_core/"
    )

    file(
        GLOB_RECURSE
        CECS_SOURCES
        CMAKE_CONFIGURE_DEPENDS
        ${CECS_CORE_SRC_DIRECTORY}/*.c
    )

    add_executable(
        ${PROJECT_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/app/src/main.c"
        ${CECS_SOURCES}
        ${CECS_MATH_SOURCES}
    )
    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC ${CECS_CORE_DIRECTORY}
        PUBLIC ${CECS_MATH_DIRECTORY}
    )

project(cecs_fiddle_test C)
    add_executable(
        ${PROJECT_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/fiddle_test/src/main.c"
        ${CECS_MATH_SOURCES}
        "${CECS_CORE_SRC_DIRECTORY}/containers/allocator/cecs_allocation.c"
        "${CECS_CORE_SRC_DIRECTORY}/containers/allocator/cecs_bump_allocator.c"
        "${CECS_CORE_SRC_DIRECTORY}/containers/allocator/cecs_arena_allocator.c"
        "${CECS_CORE_SRC_DIRECTORY}/containers/allocator/cecs_implicit_arena_allocator.c"
        "${CECS_CORE_SRC_DIRECTORY}/containers/cecs_allocator.c"
        "${CECS_CORE_SRC_DIRECTORY}/containers/cecs_dynarray.c"
        "${CECS_CORE_SRC_DIRECTORY}/containers/cecs_flatset.c"
        "${CECS_CORE_SRC_DIRECTORY}/containers/cecs_sparse_set_v2.c"
    )
    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC ${CECS_CORE_DIRECTORY}
        PUBLIC ${CECS_MATH_DIRECTORY}
    )

project(cecs_lib C)
    add_library(
        cecs
        STATIC
        ${CECS_SOURCES}
        ${CECS_MATH_SOURCES}
    )
    target_include_directories(
        cecs
        PUBLIC ${CECS_CORE_DIRECTORY}
        PUBLIC ${CECS_MATH_DIRECTORY}
    )


project(cecs_app_lib_only C)
    add_executable(
        ${PROJECT_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/app/src/main.c"
    )
    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CECS_CORE_DIRECTORY}
    )
    target_link_libraries(
        ${PROJECT_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/build/$<CONFIG>/cecs.lib"
    )


set(CECS_GRAPHICS OFF)
if(CECS_GRAPHICS)
    project(cecs_graphics C)
        add_subdirectory(
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/lib/webgpu"
        )
        add_subdirectory(
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/lib/glfw"
        )

        add_executable(
            ${PROJECT_NAME}
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/graphics_app/src/main.c"
        )
        target_include_directories(
            ${PROJECT_NAME}
            PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/src/"
        )

        target_link_libraries(
            ${PROJECT_NAME}
            PRIVATE
            webgpu
            glfw
        )
        target_copy_webgpu_binaries(${PROJECT_NAME})
endif()
