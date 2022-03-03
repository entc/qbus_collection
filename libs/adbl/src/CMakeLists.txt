#SET(CMAKE_BUILD_TYPE Debug)

#----------------------------------------------------------------------------------

SET(ADBL_CORE_SOURCES
  core/adbl.c
  core/adbl_pool.c
)

SET(ADBL_CORE_HEADERS
  core/adbl.h
  core/adbl_pool.h
  core/adbl.hpp
  core/adbl_types.h
)

#----------------------------------------------------------------------------------

INCLUDE_DIRECTORIES("." "../../cape/src")

add_library             (adbl2 SHARED ${ADBL_CORE_SOURCES} ${ADBL_CORE_HEADERS})
target_link_libraries   (adbl2 cape)
set_target_properties   (adbl2 PROPERTIES VERSION 2.0.1 SOVERSION 2.0.1)

# for local install
INSTALL (TARGETS adbl2 DESTINATION lib)

#----------------------------------------------------------------------------------

SET (PVD_ADBL_DIR ${CMAKE_CURRENT_BINARY_DIR}/pvd_adbl CACHE PATH "library path to adbl backends")
message (STATUS "PVD_ADBL_DIR  = ${PVD_ADBL_DIR}/${CMAKE_CFG_INTDIR}")

#----------------------------------------------------------------------------------

SUBDIRS(app)
SUBDIRS(pvd_mysql)
SUBDIRS(pvd_sqlite)

