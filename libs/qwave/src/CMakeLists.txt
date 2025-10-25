INCLUDE_DIRECTORIES("." "../../cape/src"  "../../qcrypt/src")

#----------------------------------------------------------------------------------

SET(QWAVE_SOURCES
  qwave.c
)

SET(QWAVE_HEADERS
  qwave.h
)

#----------------------------------------------------------------------------------

add_library             (qwave STATIC ${QWAVE_SOURCES} ${QWAVE_HEADERS})
target_link_libraries   (qwave qcrypt cape)

install (TARGETS qwave DESTINATION lib)

#----------------------------------------------------------------------------------
