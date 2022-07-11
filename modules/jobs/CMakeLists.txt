SUBDIRS(src)

file (STRINGS "version.txt" VERSION_NUMBER)
SET(CMAKE_BUILD_TYPE Release)                                     # build type

#----------------------------------------------------------------------------------

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/modules/jobs/src)

# for docker install
INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/docker/CMakeLists.txt DESTINATION ${CMAKE_BINARY_DIR}/docker/jobs/)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/jobs:${VERSION_NUMBER}_buster)")

# build the docker image with buster
INSTALL(CODE "execute_process(COMMAND docker build -t qmod/jobs:${VERSION_NUMBER}_buster -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_buster ${CMAKE_BINARY_DIR})")
