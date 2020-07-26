cmake_minimum_required(VERSION 2.4)

#----------------------------------------------------------------------------------

SET(QWEBS_CORE_SOURCES
  qwebs.c
  qwebs_connection.c
  http_parser.c
  qwebs_files.c
  qwebs_multipart.c
  qwebs_response.c
)

SET(QWEBS_CORE_HEADERS
  qwebs_structs.h
  qwebs.h
  qwebs_connection.h
  http_parser.h
  qwebs_files.h
  qwebs_multipart.h
  qwebs_response.h
)

#----------------------------------------------------------------------------------

INCLUDE_DIRECTORIES("." "../../cape/src"  "../../qcrypt/src")

add_library             (qwebs SHARED ${QWEBS_CORE_SOURCES} ${QWEBS_CORE_HEADERS})
target_link_libraries   (qwebs qcrypt cape)

install (TARGETS qwebs DESTINATION lib)

#----------------------------------------------------------------------------------
