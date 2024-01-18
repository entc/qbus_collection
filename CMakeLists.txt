PROJECT (qbus_collection)

SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_MACOSX_RPATH 0)
SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

#----------------------------------------------------------------------------------

# check for environment variable to set the etc path
if(DEFINED ENV{QBUS_ETC_DIR})
  set(_ETCDIR $ENV{QBUS_ETC_DIR})
else()
  set(_ETCDIR "base")
endif()

# try to locate the etc path
find_path (_ETCLOCATION ${_ETCDIR} HINTS ${PROJECT_SOURCE_DIR}/etc ${PROJECT_SOURCE_DIR}/../etc REQUIRED)

# set a cached variable
set(ETC_LOCATION ${_ETCLOCATION}/${_ETCDIR} CACHE PATH "configuration etc path")
message (STATUS "ETC_LOCATION = ${ETC_LOCATION}")

#----------------------------------------------------------------------------------

# basic libraries
SUBDIRS(libs/cape)
SUBDIRS(libs/adbl)
SUBDIRS(libs/qbus)
SUBDIRS(libs/qbus2)

# helper libraries for modules
SUBDIRS(libs/qcrypt)
SUBDIRS(libs/qwebs)
SUBDIRS(libs/qjobs)
SUBDIRS(libs/qflow)

# basic modules
SUBDIRS(modules/webs)
SUBDIRS(modules/auth)
SUBDIRS(modules/flow)

# tools
SUBDIRS(tools/cldd)
