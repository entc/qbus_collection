cmake_minimum_required(VERSION 2.4)

SUBDIRS(src)
#SUBDIRS(cli)
SUBDIRS(app)

execute_process(COMMAND bash config_path.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE CONFIG_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)

SET (ENV{QORE_CONFIG_PATH} ${CONFIG_PATH})

MESSAGE(STATUS "USE CONFIG PATH = '${PROJECT_SOURCE_DIR}/etc/${CONFIG_PATH}'")

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/qbus/)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus:buster)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus:buster -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile_buster ${PROJECT_SOURCE_DIR}/etc/${CONFIG_PATH}/ --build-arg CACHEBUST=0)")
