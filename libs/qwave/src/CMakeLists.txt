INCLUDE_DIRECTORIES("." "../../cape/src"  "../../qcrypt/src")

#----------------------------------------------------------------------------------

SET(QWAVE_SOURCES
  qwave.c
  qwave_conctx.c
  qwave_reqctx.c
  qwave_config.c
  qwave_response.c
  http_parser.c
)

SET(QWAVE_HEADERS
  qwave.h
  qwave_conctx.h
  qwave_reqctx.h
  qwave_config.h
  qwave_response.h
  http_parser.h
)

#----------------------------------------------------------------------------------

add_library             (qwave STATIC ${QWAVE_SOURCES} ${QWAVE_HEADERS})
target_link_libraries   (qwave qcrypt cape)

install (TARGETS qwave DESTINATION lib)

#----------------------------------------------------------------------------------
