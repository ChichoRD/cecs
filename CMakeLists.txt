cmake_minimum_required(VERSION 3.15...3.30)
enable_language(C)

set(C_STANDARD 17)
add_compile_options(
    /Zc:preprocessor
    /we\"4022\"
    /GS
    /W3
    /wd\"5045\"
    /wd\"4115\"
    /wd\"4255\"
    /wd\"4668\"
    /Zc:wchar_t
    /Zi

    /EHsc
    /nologo
)

file(
    GLOB_RECURSE
    CECS_SOURCES
    CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/cecs/*.c"
)


project(cecs_app C)
add_compile_options(
    /Fe${fileDirname}\\${fileBasenameNoExtension}.exe
)

add_executable(
    ${PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/app/src/main.c"
    ${CECS_SOURCES}
)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/cecs/"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/cecs/"
)

project(cecs_app_lib_only C)
add_compile_options(
    /Fe${fileDirname}\\${fileBasenameNoExtension}.exe
)

add_executable(
    ${PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/app/src/main.c"
)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/cecs/"
)
target_link_libraries(
    ${PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/build/$<CONFIG>/cecs.lib"
)


project(cecs_graphics C)
add_compile_options(
    /Fe${fileDirname}\\${fileBasenameNoExtension}.exe
)

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