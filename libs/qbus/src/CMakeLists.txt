# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../../qcrypt/src" "core" ".")

SUBDIRS(core)
SUBDIRS(adapter/java)
SUBDIRS(adapter/python)
SUBDIRS(engines/tcp)
SUBDIRS(tests)

set(WSRV_SOURCES
  qbus.c
)

set(WSRV_HEADERS
  qbus.h
  qbus.hpp
  qbus_types.h
)

#----------------------------------------------------------------------------------

add_library             (qbus SHARED ${WSRV_SOURCES} ${WSRV_HEADERS})
target_link_libraries   (qbus qbus_core qbus_engine_tcp cape qcrypt qbus_pvd_tcp)
set_target_properties   (qbus PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

INSTALL (TARGETS qbus DESTINATION lib)

#----------------------------------------------------------------------------------

# for local install
INSTALL (TARGETS qbus DESTINATION lib)

#----------------------------------------------------------------------------------

SET (PVD_QBUS_DIR ${CMAKE_CURRENT_BINARY_DIR}/pvd_qbus CACHE PATH "library path to qbus engines")
message (STATUS "PVD_QBUS_DIR  = ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR}")

SET (ADM_QBUS_DIR ${CMAKE_CURRENT_BINARY_DIR}/adm_qbus CACHE PATH "library path to qbus adapters")
message (STATUS "ADM_QBUS_DIR  = ${ADM_QBUS_DIR}/${CMAKE_CFG_INTDIR}")

#----------------------------------------------------------------------------------

SUBDIRS(pvd_example)
SUBDIRS(pvd_tcp)
