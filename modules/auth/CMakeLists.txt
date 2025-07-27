SUBDIRS(src)

file (STRINGS "version.txt" VERSION_NUMBER)
SET(CMAKE_BUILD_TYPE Debug)                                     # build type

#----------------------------------------------------------------------------------

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/modules/auth/src)
INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/docker/CMakeLists.txt DESTINATION ${CMAKE_BINARY_DIR}/docker)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/auth:${VERSION_NUMBER}_bookworm)")

# build the docker image with buster
INSTALL(CODE "execute_process(COMMAND docker build -t qmod/auth:${VERSION_NUMBER}_bookworm -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_bookworm ${CMAKE_BINARY_DIR})")
