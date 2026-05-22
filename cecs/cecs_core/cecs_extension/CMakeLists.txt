cmake_minimum_required(VERSION 3.30)

# HACK: cecs_extennsion bundles all current extensions and provides a shortcut for linking with them
# In the future, the extension directories can be easily split into per-extension library targets
project(
    cecs_extension
    VERSION 0.1.0
    DESCRIPTION "Extension library for CECS, providing additional features and utilities"
    LANGUAGES C
)
    option(CECS_EXTENSION_BUILD_LEGACY "Whether to build legacy extensions (e.g. cecs_group, cecs_system)" OFF)
    option(CECS_EXTENSION_BUILD_QUERY "Whether to build query extensions (i.e. cecs_query)" ON)


    include(FetchContent)
    if (${CECS_EXTENSION_BUILD_LEGACY})
        FetchContent_Declare(
            cecs_world
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cecs_world
        )
        FetchContent_MakeAvailable(cecs_world)
    endif()
    if (${CECS_EXTENSION_BUILD_QUERY})
        FetchContent_Declare(
            cecs_query
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cecs_query
        )
        FetchContent_MakeAvailable(cecs_query)
    endif()


    # FIXME: should be INTERFACE?
    add_library(${PROJECT_NAME} INTERFACE)
    if (${CECS_EXTENSION_BUILD_LEGACY})
        target_link_libraries(${PROJECT_NAME} INTERFACE cecs_world)
    endif()
    if (${CECS_EXTENSION_BUILD_QUERY})
        target_link_libraries(${PROJECT_NAME} INTERFACE cecs_query)
    endif()

    
    if (${CECS_EXTENSION_BUILD_LEGACY})
        target_sources(
            ${PROJECT_NAME}
            PRIVATE
                src/cecs_group/src/group/cecs_sparse_set_group.c
                src/cecs_group/src/cecs_group_storage.c


                src/cecs_query/src/query/descriptor/cecs_query_common.c
                src/cecs_query/src/query/cecs_query_shared.c
                src/cecs_query/src/query/cecs_query_descriptor.c
                src/cecs_query/src/cecs_query.c


                # src/cecs_system/src/cecs_system.c
            PUBLIC
                FILE_SET ${PROJECT_NAME}_HEADERS
                TYPE HEADERS
                BASE_DIRS 
                    src/cecs_group/src
                    src/cecs_query/src
                    src/cecs_system/src
                FILES
                    src/cecs_group/src/group/cecs_sparse_set_group.h
                    src/cecs_group/src/cecs_group_storage.h
                    src/cecs_group/src/cecs_group.h


                    src/cecs_query/src/query/descriptor/cecs_query_common.h
                    src/cecs_query/src/query/descriptor/cecs_query_descriptor_shared.h
                    # src/cecs_query/src/query/descriptor/cecs_query_descriptor_partial.h
                    # src/cecs_query/src/query/descriptor/cecs_query_descriptor_exclusive.h

                    src/cecs_query/src/query/cecs_query_descriptor.h
                    src/cecs_query/src/query/cecs_query_shared.h
                    # src/cecs_query/src/query/cecs_query_partial.h
                    # src/cecs_query/src/query/cecs_query_exclusive.h

                    src/cecs_query/src/cecs_query.h


                    src/cecs_system/src/cecs_system.h
        )
    endif()