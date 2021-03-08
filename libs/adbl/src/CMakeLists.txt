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

INCLUDE_DIRECTORIES("." "core")

SET(ADBL_APP_SOURCES
  app/adbl_app.c
)

add_executable          (adbl_app ${ADBL_APP_SOURCES})
target_link_libraries   (adbl_app adbl2)
set_target_properties   (adbl_app PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

#INSTALL (TARGETS adbl_app DESTINATION bin)

# for docker
#INSTALL (TARGETS adbl_app DESTINATION docker/qore/bin)

#----------------------------------------------------------------------------------

SUBDIRS(pvd_mysql)
SUBDIRS(pvd_sqlite)

