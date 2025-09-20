SUBDIRS(src)

file (STRINGS "version.txt" VERSION_NUMBER)

#----------------------------------------------------------------------------------

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/modules/flow/src)

#----------------------------------------------------------------------------------

INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/docker/CMakeLists_min.txt DESTINATION ${CMAKE_BINARY_DIR}/docker RENAME 2fd0e638-9489-410f-9d38-dbcacd5b15b6.txt)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qmod/flow:${VERSION_NUMBER}_min)")

# build the docker image with buster
INSTALL(CODE "execute_process(COMMAND docker build --platform=linux/amd64 -t qmod/flow:${VERSION_NUMBER}_min -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_min ${CMAKE_BINARY_DIR})")

#----------------------------------------------------------------------------------
