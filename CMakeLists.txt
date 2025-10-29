cmake_minimum_required(VERSION 3.15...3.30)
if (MSVC)
    add_compile_options(
        /Wall /WX
    )
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic -Werror
        -mlzcnt -mavx -mavx2
    )
endif()

set(
    CECS_CORE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/cecs_core/"
)
set(
    CECS_CORE_SRC_DIRECTORY
    "${CECS_CORE_DIRECTORY}/cecs_core/"
)

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

project(cecs_fiddle_test C)
    add_executable(
        ${PROJECT_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/fiddle_test/src/main.c"
        ${CECS_MATH_SOURCES}
        "${CECS_CORE_SRC_DIRECTORY}/cecs_error.c"
        "${CECS_CORE_SRC_DIRECTORY}/sync/cecs_rwlock.c"
        "${CECS_CORE_SRC_DIRECTORY}/allocator/cecs_allocation.c"
        "${CECS_CORE_SRC_DIRECTORY}/allocator/cecs_memory.c"
        "${CECS_CORE_SRC_DIRECTORY}/allocator/cecs_bump_allocator.c"
        "${CECS_CORE_SRC_DIRECTORY}/allocator/cecs_arena_allocator.c"
        "${CECS_CORE_SRC_DIRECTORY}/allocator/cecs_implicit_arena_allocator.c"
        "${CECS_CORE_SRC_DIRECTORY}/cecs_allocator.c"
        "${CECS_CORE_SRC_DIRECTORY}/container/cecs_dynarray.c"
        "${CECS_CORE_SRC_DIRECTORY}/container/cecs_flatbucket.c"
        "${CECS_CORE_SRC_DIRECTORY}/container/cecs_flatset.c"
        "${CECS_CORE_SRC_DIRECTORY}/container/cecs_flatmap.c"
        "${CECS_CORE_SRC_DIRECTORY}/container/cecs_sparse_set.c"
        "${CECS_CORE_SRC_DIRECTORY}/world/cecs_entity.c"
        "${CECS_CORE_SRC_DIRECTORY}/world/cecs_entity_storage.c"
        "${CECS_CORE_SRC_DIRECTORY}/world/registry/cecs_component_storage.c"
        "${CECS_CORE_SRC_DIRECTORY}/world/registry/component_storage/cecs_sparse_set_storage.c"
        "${CECS_CORE_SRC_DIRECTORY}/world/cecs_registry.c"
        "${CECS_CORE_SRC_DIRECTORY}/world/cecs_world_components.c"
        "${CECS_CORE_SRC_DIRECTORY}/world/cecs_view.c"
        "${CECS_CORE_SRC_DIRECTORY}/world/group/cecs_sparse_set_group.c"
        "${CECS_CORE_SRC_DIRECTORY}/cecs_world.c"
    )
    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC ${CECS_CORE_DIRECTORY}
        PUBLIC ${CECS_MATH_DIRECTORY}
    )

# project(cecs_lib C)
#     add_library(
#         cecs
#         STATIC
#         ${CECS_SOURCES}
#         ${CECS_MATH_SOURCES}
#     )
#     target_include_directories(
#         cecs
#         PUBLIC ${CECS_CORE_DIRECTORY}
#         PUBLIC ${CECS_MATH_DIRECTORY}
#     )

# project(cecs_app C)
#     file(
#         GLOB_RECURSE
#         CECS_SOURCES
#         CMAKE_CONFIGURE_DEPENDS
#         ${CECS_CORE_SRC_DIRECTORY}/*.c
#     )

#     add_executable(
#         ${PROJECT_NAME}
#         "${CMAKE_CURRENT_SOURCE_DIR}/examples/app/src/main.c"
#         ${CECS_SOURCES}
#         ${CECS_MATH_SOURCES}
#     )
#     target_include_directories(
#         ${PROJECT_NAME}
#         PUBLIC ${CECS_CORE_DIRECTORY}
#         PUBLIC ${CECS_MATH_DIRECTORY}
#     )

# project(cecs_app_lib_only C)
#     add_executable(
#         ${PROJECT_NAME}
#         "${CMAKE_CURRENT_SOURCE_DIR}/examples/app/src/main.c"
#     )
#     target_include_directories(
#         ${PROJECT_NAME}
#         PUBLIC
#         ${CECS_CORE_DIRECTORY}
#     )
#     target_link_libraries(
#         ${PROJECT_NAME}
#         "${CMAKE_CURRENT_SOURCE_DIR}/build/$<CONFIG>/cecs.lib"
#     )


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
