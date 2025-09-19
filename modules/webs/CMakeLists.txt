SUBDIRS(src)

file (STRINGS "version.txt" VERSION_NUMBER)

#----------------------------------------------------------------------------------

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/modules/webs/src)

#----------------------------------------------------------------------------------

INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/docker/CMakeLists_min.txt DESTINATION ${CMAKE_BINARY_DIR}/docker RENAME 9e54a2c8-cea9-4569-b618-ba99cc6da9c8.txt)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/webs:${VERSION_NUMBER}_min)")

# build the docker image with buster
INSTALL(CODE "execute_process(COMMAND docker build --platform=linux/amd64 -t qmod/webs:${VERSION_NUMBER}_min -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_min ${CMAKE_BINARY_DIR})")

#----------------------------------------------------------------------------------
