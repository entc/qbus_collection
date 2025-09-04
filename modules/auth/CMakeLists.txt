SUBDIRS(src)

file (STRINGS "version.txt" VERSION_NUMBER)
SET(CMAKE_BUILD_TYPE Debug)                                     # build type

#----------------------------------------------------------------------------------

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/modules/auth/src)
INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/docker/CMakeLists_min.txt DESTINATION ${CMAKE_BINARY_DIR}/docker RENAME CMakeLists.txt)

#----------------------------------------------------------------------------------

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/auth:${VERSION_NUMBER}_min)")

# build the docker image with buster
INSTALL(CODE "execute_process(COMMAND docker build --platform=linux/amd64 -t qmod/auth:${VERSION_NUMBER}_min -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_min ${CMAKE_BINARY_DIR})")

#----------------------------------------------------------------------------------

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/auth:${VERSION_NUMBER}_debug)")

# build the docker image with buster
INSTALL(CODE "execute_process(COMMAND docker build --platform=linux/amd64 -t qmod/auth:${VERSION_NUMBER}_debug -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_debug ${CMAKE_BINARY_DIR})")
