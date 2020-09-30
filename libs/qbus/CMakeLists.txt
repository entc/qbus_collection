cmake_minimum_required(VERSION 2.4)

SUBDIRS(src)
#SUBDIRS(cli)
SUBDIRS(app)

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/qbus/)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus:buster)")

# build the docker image
#INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus:buster -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile_buster ${PROJECT_SOURCE_DIR}/etc/$CONFIG_PATH/ --build-arg CACHEBUST=0)")
INSTALL(CODE "execute_process(COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/docker_build.sh ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR})")
