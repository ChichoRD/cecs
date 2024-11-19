project(cecs)

set(CMAKE_CXX_STANDARD 17)
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
    /Fe${fileDirname}\\${fileBasenameNoExtension}.exe
)

file(
	GLOB
	SOURCES
	CMAKE_CONFIGURE_DEPENDS
	include/*.c
    include/*.h
    include/containers/*.c
    include/containers/*.h
    include/core/*.c
    include/core/*.h
    include/core/component/*.c
    include/core/component/*.h
    include/core/component/entity/*.c
    include/core/component/entity/*.h
    include/core/resource/*.c
    include/core/resource/*.h
    include/types/*.c
    include/types/*.h

)


# add_link_options(
# 	/VERBOSE
# )

add_executable(cecs
	# Archivo de código
	src/main.c
	#src/game.cpp
	${SOURCES}
)
