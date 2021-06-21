SUBDIRS(src)
#SUBDIRS(cli)
SUBDIRS(app)

if(NOT DEFINED ENV{QBUS_ETC_DIR})

    set(ENV{QBUS_ETC_DIR} "base")
    message (STATUS "QBUS_ETC_DIR  = ${PROJECT_SOURCE_DIR}/etc/$ENV{QBUS_ETC_DIR}")

endif()

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/qbus/)

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus:buster)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus:buster -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile_buster ${CMAKE_INSTALL_PREFIX}/ --build-arg CACHEBUST=0)")

#--------------------- OLD VERSION --------------------------

# remove none used ones (needs to fix)
#INSTALL(CODE "execute_process(COMMAND docker image rm qore:1.0.0)")

# build the docker image
#INSTALL(CODE "execute_process(COMMAND docker build -t qore:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")
