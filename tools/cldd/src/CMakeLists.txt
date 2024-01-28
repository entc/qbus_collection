# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qcrypt/src")
SET(CMAKE_BUILD_TYPE Release)                                   # build type

add_executable          (cldd cldd.c)
target_link_libraries   (cldd cape qcrypt)

SET (TOOL_CLDD ${CMAKE_CURRENT_BINARY_DIR}/cldd CACHE PATH "path to ldd tool")
