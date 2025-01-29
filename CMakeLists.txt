cmake_minimum_required(VERSION 3.15...3.30)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)


enable_language(C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (MSVC)
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
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic

        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-newline-eof
        -Wno-unused-function
        -Wno-extra-semi
        #-Werror
    )
endif()


project(cecs_math C)
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
    )
    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CECS_CORE_DIRECTORY}
    )


project(cecs_lib C)
    add_library(
        cecs
        STATIC
        ${CECS_SOURCES}
    )
    target_include_directories(
        cecs
        PUBLIC
        ${CECS_CORE_DIRECTORY}
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


set(CECS_GRAPHICS ON)
if(CECS_GRAPHICS)
    project(cecs_graphics C)
        file(
            GLOB_RECURSE
            CECS_GRAPHICS_SOURCES
            CMAKE_CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/cecs_graphics/*.c"
        )

        add_subdirectory(
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/lib/webgpu"
        )
        add_subdirectory(
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/lib/glfw"
        )
        add_subdirectory(
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/lib/glfw3webgpu"
        )

        add_executable(
            ${PROJECT_NAME}
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/graphics_app/src/main.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/graphics_app/src/test_pass.c"
            ${CECS_GRAPHICS_SOURCES}
        )
        target_include_directories(
            ${PROJECT_NAME}
            PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/cecs_graphics/"
            "${CMAKE_CURRENT_SOURCE_DIR}/cecs_graphics/lib/"
            ${CECS_CORE_DIRECTORY}
            ${CECS_MATH_DIRECTORY}
        )
        target_link_libraries(
            ${PROJECT_NAME}
            PUBLIC
            cecs
        )
        
        target_link_libraries(
            ${PROJECT_NAME}
            PRIVATE
            webgpu
            glfw
            glfw3webgpu
        )
        target_copy_webgpu_binaries(${PROJECT_NAME})

        if (MSVC)
            target_compile_options(${PROJECT_NAME} PUBLIC /wd4244)
        endif()
endif()
