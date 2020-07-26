cmake_minimum_required(VERSION 2.4)

SUBDIRS(src)
#SUBDIRS(cli)
SUBDIRS(app)

# copy into tmp folder all files needed

# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qore/qbus:jessie)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t qore/qbus:jessie -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_CURRENT_SOURCE_DIR})")
