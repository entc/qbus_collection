SUBDIRS(src)
SUBDIRS(app)
SUBDIRS(tests)

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/qbus/)

# remove none used ones
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus2:bullseye)")
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus2:bookworm)")

SET(build_bookworm "execute_process(COMMAND docker build -t qore/qbus2:bookworm ${PROXY_DOCKER} --platform linux/amd64 -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_bookworm ${CMAKE_INSTALL_PREFIX}/)")

INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus2:bullseye --build-arg http_proxy=\"http:\/\/proxywww.nos.office:3128\" --platform linux/amd64 -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_bullseye ${CMAKE_INSTALL_PREFIX}/)")

message (STATUS "Build Bookworm = ${build_bookworm}")
INSTALL(CODE ${build_bookworm})
