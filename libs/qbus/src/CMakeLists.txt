INCLUDE_DIRECTORIES("../../cape/src" "../../qcrypt/src" "pvd_common")

# pvd engines
SUBDIRS(pvd_common)
SUBDIRS(pvd_mqtt)

# adapters and examples
SUBDIRS(adapter/java)
SUBDIRS(adapter/python)
SUBDIRS(adapter_example)

#----------------------------------------------------------------------------------------

SET (QBUS_LIBRARY_SOURCES
  "qbus.c"
  "qbus_engines.c"
  "qbus_router.c"
  "qbus_con.c"
  "qbus_snd.c"
  "qbus_message.c"
  "qbus_config.c"
  "qbus_methods.c"
  "qbus_agent.c"
)

SET (QBUS_LIBRARY_HEADERS
  "qbus.h"
  "qbus_engines.h"
  "qbus_router.h"
  "qbus_con.h"
  "qbus_snd.h"
  "qbus_message.h"
  "qbus_config.h"
  "qbus_methods.h"
  "qbus_agent.h"
)

add_library             (qbus SHARED ${QBUS_LIBRARY_SOURCES} ${QBUS_LIBRARY_HEADERS})
target_link_libraries   (qbus cape qbus_pvd_common)

# for local install
INSTALL (TARGETS qbus DESTINATION lib)

#----------------------------------------------------------------------------------

SET (PVD_QBUS_DIR ${CMAKE_CURRENT_BINARY_DIR}/pvd_qbus CACHE PATH "library path to qbus engines")
message (STATUS "PVD_QBUS_DIR  = ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR}")

SET (ADM_QBUS_DIR ${CMAKE_CURRENT_BINARY_DIR}/adm_qbus CACHE PATH "library path to qbus adapters")
message (STATUS "ADM_QBUS_DIR  = ${ADM_QBUS_DIR}/${CMAKE_CFG_INTDIR}")

#----------------------------------------------------------------------------------
