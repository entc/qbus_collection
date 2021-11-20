PROJECT (qbus_collection)

SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_MACOSX_RPATH 0)
SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

#----------------------------------------------------------------------------------

# basic libraries
SUBDIRS(libs/cape)
SUBDIRS(libs/adbl)
SUBDIRS(libs/qbus)

# helper libraries for modules
SUBDIRS(libs/qcrypt)
SUBDIRS(libs/qwebs)
SUBDIRS(libs/qjobs)
SUBDIRS(libs/qflow)

# basic modules
SUBDIRS(modules/webs)
SUBDIRS(modules/auth)
SUBDIRS(modules/flow)
