cmake_minimum_required(VERSION 3.30)

project(
    cecs_math
    VERSION 0.1.0
    DESCRIPTION "Mathematical functions library for CECS"
    LANGUAGES C
)
    set(CECS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
    include(FetchContent)
    FetchContent_Declare(
        cecs_meta
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_meta
    )
    FetchContent_Declare(
        cecs_compile_options
        SOURCE_DIR ${CECS_SOURCE_DIR}/cecs/cecs_compile_options
    )
    FetchContent_MakeAvailable(cecs_compile_options cecs_meta)
    add_library(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE cecs_compile_options cecs_meta)

    if (NOT MSVC)
        # required for: lzcnt
        cecs_add_c_compiler_option(OPTION "-mlzcnt" TARGET ${PROJECT_NAME} VISIBILITY PUBLIC REQUIRED)
        # required for: tzcnt
        cecs_add_c_compiler_option(OPTION "-mbmi" TARGET ${PROJECT_NAME} VISIBILITY PUBLIC REQUIRED)
    endif()
    
    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            src/algebra/complex/cecs_quaternion.c
            src/algebra/linear/cecs_matrix.c
            src/algebra/linear/cecs_vector.c
            src/algebra/cecs_spatial.c

            src/arithmetic/cecs_integer_arithmetic.c

            src/relations/cecs_ordering.c

            src/cecs_math.c
        PUBLIC
            FILE_SET ${PROJECT_NAME}_HEADERS
            TYPE HEADERS
            BASE_DIRS src
            FILES
                src/algebra/complex/cecs_quaternion.h
                src/algebra/linear/cecs_matrix.h
                src/algebra/linear/cecs_vector_def.h
                src/algebra/linear/cecs_vector.h
                src/algebra/cecs_complex.h
                src/algebra/cecs_spatial.h

                src/arithmetic/cecs_integer_arithmetic.h

                src/relations/cecs_ordering.h

                src/cecs_math.h
    )