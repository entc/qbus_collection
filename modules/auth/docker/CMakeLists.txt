cmake_minimum_required(VERSION 2.9)

PROJECT(qmod_auth)

SET(CMAKE_POSITION_INDEPENDENT_CODE ON)                         # activates -fPic on linux
SET(CMAKE_MACOSX_RPATH 0)                                       # strip away rpath
SET(CMAKE_BUILD_TYPE Debug)                                     # build type
SET(CMAKE_ENABLE_EXPORTS ON)
SET(CMAKE_VERBOSE_MAKEFILE OFF)

if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif ()
else ()
  set (CMAKE_CXX_STANDARD 11)
endif ()

SET(CMAKE_INSTALL_PREFIX /opt/)

# basic libs
ADD_SUBDIRECTORY(qbus_collection/libs/cape/src)
ADD_SUBDIRECTORY(qbus_collection/libs/adbl/src)
ADD_SUBDIRECTORY(qbus_collection/libs/qbus/src)
ADD_SUBDIRECTORY(qbus_collection/libs/qcrypt/src)
ADD_SUBDIRECTORY(qbus_collection/libs/qjobs/src)

# tools
ADD_SUBDIRECTORY(qbus_collection/tools/cldd/src)

# module
ADD_SUBDIRECTORY(qbus_collection/modules/auth/src)

message (STATUS "TOOL_CLDD  = ${TOOL_CLDD}")
message (STATUS "PVD_QBUS_DIR  = ${PVD_QBUS_DIR}")

INSTALL(CODE "execute_process(COMMAND bash -c \"${TOOL_CLDD} ${CMAKE_CURRENT_BINARY_DIR}/qbus_collection/modules/auth/src/${CMAKE_CFG_INTDIR}/qbus_mod_auth ${CMAKE_CURRENT_BINARY_DIR}/image_gen\")")
INSTALL(CODE "execute_process(COMMAND bash -c \"${TOOL_CLDD} ${PVD_ADBL_DIR}/libadbl2_mysql.so ${CMAKE_CURRENT_BINARY_DIR}/image_gen adbl\")")

# for debuging
INSTALL(CODE "execute_process(COMMAND bash -c \"${TOOL_CLDD} /usr/bin/valgrind.bin ${CMAKE_CURRENT_BINARY_DIR}/image_gen\")")
