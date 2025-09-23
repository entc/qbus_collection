# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qcrypt/src")
SET(CMAKE_BUILD_TYPE Release)                                   # build type

add_executable          (execsec execsec.c)
target_link_libraries   (execsec cape qcrypt)

SET (TOOL_EXECSEC ${CMAKE_CURRENT_BINARY_DIR}/execsec CACHE FILEPATH "path to execsec tool" FORCE)
