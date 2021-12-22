SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
SET(CMAKE_MACOSX_RPATH 0)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
#SET(CMAKE_BUILD_TYPE Debug)

find_package(AdblSqlite3)

IF(SQLITE_FOUND)

  INCLUDE_DIRECTORIES( "../core" ${SQLITE_INCLUDES})

  SET (CORE_SOURCES
    adbl_sqlite.c
    prepare.c
  )

  SET (CORE_HEADERS
    prepare.h
  )

  ADD_LIBRARY(adbl2_sqlite3 SHARED ${CORE_SOURCES} ${CORE_HEADERS})
  TARGET_LINK_LIBRARIES( adbl2_sqlite3 adbl2 cape ${SQLITE_LIBRARIES})
  SET_TARGET_PROPERTIES (adbl2_sqlite3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PVD_ADBL_DIR})

  INSTALL (TARGETS adbl2_sqlite3 DESTINATION mods/adbl)

  #--------------------------------------------------------------
  # unit tests 
  
  INCLUDE_DIRECTORIES( "." ${SQLITE_INCLUDES})

  add_executable          (ut_basic_sqlite "tests/ut_basic.c")
  target_link_libraries   (ut_basic_sqlite adbl2_sqlite3)

ELSE()

  MESSAGE(WARNING "sqlite3 was not found on your system" )

ENDIF()
