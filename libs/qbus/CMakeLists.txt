cmake_minimum_required(VERSION 2.4)

SUBDIRS(src)
#SUBDIRS(cli)
SUBDIRS(app)

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/qbus/)

# copy into tmp folder all files needed
SET(QORE_CONFIG_PATH ${PROJECT_SOURCE_DIR}/config/$ENV{QORE_CONFIG_PATH})

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus:jessie)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus:jessie -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${QORE_CONFIG_PATH})")
