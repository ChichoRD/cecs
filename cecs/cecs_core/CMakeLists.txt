cmake_minimum_required(VERSION 3.30)

project(
    cecs_core
    VERSION 0.1.0
    DESCRIPTION "Core library for CECS"
    LANGUAGES C
)
    include(FetchContent)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
    FetchContent_Declare(
        cecs_meta
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_meta
    )
    FetchContent_Declare(
        cecs_compile_options
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_compile_options
    )
    FetchContent_Declare(
        cecs_error
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_error
    )
    FetchContent_Declare(
        cecs_lib
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_lib
    )
    FetchContent_Declare(
        cecs_sync
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_sync
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_meta cecs_error cecs_lib cecs_sync)

    add_library(${PROJECT_NAME})
    add_library(cecs ALIAS ${PROJECT_NAME})
    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
            cecs_compile_options cecs_meta cecs_error
        PUBLIC
            cecs_lib # XXX: linking targets link automatically to cecs_libs too
            cecs_sync
    )

    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/world/group/cecs_sparse_set_group.c

            src/world/registry/component_storage/cecs_sparse_set_storage.c
            src/world/registry/cecs_component_storage.c

            src/world/cecs_component_registry.c
            src/world/cecs_entity_storage.c
            src/world/cecs_entity.c
            src/world/cecs_group_storage.c
            src/world/cecs_registry.c
            src/world/cecs_view.c
            src/world/cecs_world_components.c

            src/cecs_world.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/world/group/cecs_sparse_set_group.h

                src/world/registry/component_storage/cecs_sparse_set_storage.h
                src/world/registry/cecs_component_storage.h

                src/world/cecs_component_registry.h
                src/world/cecs_entity_storage.h
                src/world/cecs_entity.h
                src/world/cecs_group_storage.h
                src/world/cecs_group.h
                src/world/cecs_registry.h
                src/world/cecs_system.h
                src/world/cecs_view.h
                src/world/cecs_world_components.h

                src/cecs_world.h

    )