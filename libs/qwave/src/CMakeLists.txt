INCLUDE_DIRECTORIES("." "../../cape/src"  "../../qcrypt/src")

#----------------------------------------------------------------------------------

SET(QWAVE_SOURCES
  qwave.c
  qwave_aioctx.c
  qwave_conctx.c
  qwave_config.c
  http_parser.c
)

SET(QWAVE_HEADERS
  qwave.h
  qwave_aioctx.h
  qwave_conctx.h
  qwave_config.h
  http_parser.h
)

#----------------------------------------------------------------------------------

add_library             (qwave STATIC ${QWAVE_SOURCES} ${QWAVE_HEADERS})
target_link_libraries   (qwave qcrypt cape)

install (TARGETS qwave DESTINATION lib)

#----------------------------------------------------------------------------------
