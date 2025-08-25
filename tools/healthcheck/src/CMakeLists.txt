# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qcrypt/src")
SET(CMAKE_BUILD_TYPE Release)                               # build type

add_executable          (healthcheck healthcheck.c)
target_link_libraries   (healthcheck cape qcrypt)

SET (TOOL_HEALTHCHECK ${CMAKE_CURRENT_BINARY_DIR}/healthcheck CACHE FILEPATH "path to healthcheck tool" FORCE)
