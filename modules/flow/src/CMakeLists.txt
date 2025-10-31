INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qbus/src" "../../../libs/adbl/src/core" "../../../libs/qflow/src" "../../../libs/qjobs/src" "../../../libs/qcrypt/src" "../../../libs/qtee/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_FLOW_SOURCES
  "flow.c"
  "flow_workflow.c"
  "flow_workstep.c"
  "flow_process.c"
  "flow_chain.c"
  "flow_run_dbw.c"
  "flow_run_step.c"
  "flow_run.c"
)

SET (QBUS_MODULES_FLOW_HEADERS
  "flow_workflow.h"
  "flow_workstep.h"
  "flow_process.h"
  "flow_chain.h"
  "flow_run_dbw.h"
  "flow_run_step.h"
  "flow_run.h"
)

ADD_EXECUTABLE (qbus_mod_flow ${QBUS_MODULES_FLOW_SOURCES} ${QBUS_MODULES_FLOW_HEADERS})
TARGET_LINK_LIBRARIES (qbus_mod_flow qbus qflow adbl2 qjobs qtee)

# for local install
INSTALL (TARGETS qbus_mod_flow DESTINATION mods)
add_custom_command (TARGET qbus_mod_flow POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/*.sh ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

if (ETC_LOCATION)
  add_custom_command (TARGET qbus_mod_flow POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

if (PVD_ADBL_DIR)
  add_custom_command (TARGET qbus_mod_flow POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_ADBL_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
endif()

if (PVD_QBUS_DIR)
  add_custom_command (TARGET qbus_mod_flow POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()

#----------------------------------------------------------------------------------
