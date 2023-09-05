cmake_minimum_required(VERSION 2.4)

PROJECT(globeon)

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

SUBDIRS(../qbus_collection/libs/cape/src)
SUBDIRS(../qbus_collection/libs/adbl/src)
SUBDIRS(../qbus_collection/libs/qcrypt/src)
SUBDIRS(../qbus_collection/libs/qbus/src)
SUBDIRS(../qbus_collection/libs/qflow/src)
SUBDIRS(../qbus_collection/libs/qjobs/src)

SUBDIRS(../qbus_collection/modules/flow/src)
