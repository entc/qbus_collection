# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "core" ".")

SUBDIRS(core)
SUBDIRS(adapter/java)
SUBDIRS(adapter/python)
SUBDIRS(engines/tcp)

set(WSRV_SOURCES
  qbus.c
)

set(WSRV_HEADERS
  qbus.h
  qbus.hpp
)

#----------------------------------------------------------------------------------

add_library             (qbus SHARED ${WSRV_SOURCES} ${WSRV_HEADERS})
target_link_libraries   (qbus qbus_core qbus_engine_tcp cape)
set_target_properties   (qbus PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

INSTALL (TARGETS qbus DESTINATION lib)

#----------------------------------------------------------------------------------

# for local install
INSTALL (TARGETS qbus DESTINATION lib)

#----------------------------------------------------------------------------------
