SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
SET(CMAKE_MACOSX_RPATH 0)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(CMAKE_BUILD_TYPE Debug)

find_package(AdblMysql QUIET)

IF(MYSQL_FOUND)

if (MSVC)
  find_package(Zlib REQUIRED)
  message (STATUS "ZLIB_LIBRARIES = ${ZLIB_LIBRARIES}")
endif()
 
  INCLUDE_DIRECTORIES( "../core" ${MYSQL_INCLUDES})

  SET (CORE_SOURCES
    adbl_mysql.c
    bindvars.c
    prepare.c
  )

  SET (CORE_HEADERS
    adbl_mysql.h
    bindvars.h
    prepare.h
  )

  ADD_LIBRARY(adbl2_mysql SHARED ${CORE_SOURCES} ${CORE_HEADERS})
  TARGET_LINK_LIBRARIES( adbl2_mysql adbl2 cape ${MYSQL_LIBRARIES} ${ZLIB_LIBRARIES})
  SET_TARGET_PROPERTIES (adbl2_mysql PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PVD_ADBL_DIR})

  INSTALL (TARGETS adbl2_mysql DESTINATION mods/adbl)

  #--------------------------------------------------------------
  # unit tests 
  
  INCLUDE_DIRECTORIES( "." ${MYSQL_INCLUDES})

  add_executable          (ut_basic_mysql "tests/ut_basic.c")
  target_link_libraries   (ut_basic_mysql adbl2_mysql)

  
ELSE(MYSQL_FOUND)

  MESSAGE(WARNING "mysqlclient was not found on your system" )

ENDIF(MYSQL_FOUND)
