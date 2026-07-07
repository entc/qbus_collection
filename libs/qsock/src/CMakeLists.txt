INCLUDE_DIRECTORIES("." "../../cape/src")

#----------------------------------------------------------------------------------

SET(QSOCK_SOURCES
  qsock_client.c
)

SET(QSOCK_HEADERS
  qsock_client.h
)

#----------------------------------------------------------------------------------

add_library             (qsock STATIC ${QSOCK_SOURCES} ${QSOCK_HEADERS})
target_link_libraries   (qsock cape)

install (TARGETS qsock DESTINATION lib)

#----------------------------------------------------------------------------------
