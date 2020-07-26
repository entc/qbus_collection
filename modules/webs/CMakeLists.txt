cmake_minimum_required(VERSION 2.4)

SUBDIRS(src)

#----------------------------------------------------------------------------------

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ DESTINATION ${CMAKE_BINARY_DIR}/docker/modules/webs/src)

# for docker install
INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/docker/CMakeLists.txt DESTINATION ${CMAKE_BINARY_DIR}/docker/)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/webs:jessie)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t qmod/webs:jessie -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile ${CMAKE_BINARY_DIR})")
