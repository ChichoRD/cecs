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
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.c"
)


project(cecs_app C)
add_compile_options(
    /Fe${fileDirname}\\${fileBasenameNoExtension}.exe
)

add_executable(
    ${PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
    ${CECS_SOURCES}
)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
)