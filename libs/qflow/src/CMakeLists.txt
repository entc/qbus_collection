
#----------------------------------------------------------------------------------

SET(QFLOW_CORE_SOURCES
  qflow.c
)

SET(QFLOW_CORE_HEADERS
  qflow.h
)

#----------------------------------------------------------------------------------

INCLUDE_DIRECTORIES("." "../../cape/src" "../../adbl/src/core")

add_library             (qflow SHARED ${QFLOW_CORE_SOURCES} ${QFLOW_CORE_HEADERS})
target_link_libraries   (qflow cape adbl2)

#----------------------------------------------------------------------------------
