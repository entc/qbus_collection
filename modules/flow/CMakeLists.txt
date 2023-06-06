SUBDIRS(src)

file (STRINGS "version.txt" VERSION_NUMBER)

#----------------------------------------------------------------------------------

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/modules/flow/src)

# for docker install
INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/docker/CMakeLists.txt DESTINATION ${CMAKE_BINARY_DIR}/docker/flow/)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/flow:${VERSION_NUMBER}_bullseye)")

# build the docker image with bullseye
INSTALL(CODE "execute_process(COMMAND docker build -t qmod/flow:${VERSION_NUMBER}_bullseye -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_bullseye ${CMAKE_BINARY_DIR})")
