SUBDIRS(src)

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/qbus/)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus2:buster)")
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus2:bullseye)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus2:buster -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_buster ${CMAKE_INSTALL_PREFIX}/ --build-arg CACHEBUST=0)")
INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus2:bullseye -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_bullseye ${CMAKE_INSTALL_PREFIX}/ --build-arg CACHEBUST=0)")
