# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../../qcrypt/src" "core" ".")

subdirs(tl1/lib)
subdirs(tl1/emu)
#subdirs(tl2/lib)

set(QBUS_SOURCES
  qbus.c
  qbus_method.c
  qbus_route.c
  qbus_storage.c
)

set(QBUS_HEADERS
  qbus.h
  qbus_method.h
  qbus_route.h
  qbus_storage.h
  qbus.hpp
  qbus_types.h
)

#----------------------------------------------------------------------------------

add_library             (qbus SHARED ${QBUS_SOURCES} ${QBUS_HEADERS})
target_link_libraries   (qbus cape qcrypt qbus_tl1)

#----------------------------------------------------------------------------------

add_library             (qbus_emu SHARED ${QBUS_SOURCES} ${QBUS_HEADERS})
target_link_libraries   (qbus_emu cape qcrypt qbus_tl1_emu)

#----------------------------------------------------------------------------------

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

#SUBDIRS(pvd_example)
#SUBDIRS(pvd_tcp)
