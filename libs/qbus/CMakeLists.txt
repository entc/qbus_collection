SUBDIRS(src)
SUBDIRS(app)
SUBDIRS(tests)

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/qbus)

# remove none used ones
#INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus:buster)")
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus:bullseye)")
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus:bookworm)")

# build the docker image
#INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus:buster     -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_buster ${CMAKE_INSTALL_PREFIX}/ --build-arg CACHEBUST=0)")
INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus:bullseye   -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_bullseye ${CMAKE_INSTALL_PREFIX}/ --build-arg CACHEBUST=0)")
INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus:bookworm   -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_bookworm ${CMAKE_INSTALL_PREFIX}/ --build-arg CACHEBUST=0)")

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm mosquitto:bookworm)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t mosquitto:bookworm -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_mosquitto ${CMAKE_BINARY_DIR})")
