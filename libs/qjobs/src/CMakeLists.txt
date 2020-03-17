
#----------------------------------------------------------------------------------

SET(QFLOW_CORE_SOURCES
  qjobs.c
)

SET(QFLOW_CORE_HEADERS
  qjobs.h
)

#----------------------------------------------------------------------------------

INCLUDE_DIRECTORIES("." "../../cape/src" "../../adbl/src/core" "../../qcrypt/src")

add_library             (qjobs SHARED ${QFLOW_CORE_SOURCES} ${QFLOW_CORE_HEADERS})
target_link_libraries   (qjobs cape adbl2 qcrypt)

#----------------------------------------------------------------------------------
