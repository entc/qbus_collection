cmake_minimum_required(VERSION 2.4)

SUBDIRS(src)

#----------------------------------------------------------------------------------

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/modules/flow/src)

# for docker install
INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/docker/CMakeLists.txt DESTINATION ${CMAKE_BINARY_DIR}/docker/flow/)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/flow:buster)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t qmod/flow:buster -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_buster ${CMAKE_BINARY_DIR})")
