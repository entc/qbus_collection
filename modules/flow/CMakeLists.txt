INCLUDE_DIRECTORIES("../../libs/cape/src" "../../libs/qbus/src" "../../libs/adbl/src/core" "../../libs/qflow/src")

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
TARGET_LINK_LIBRARIES(qbus_mod_flow qbus qflow adbl2)

# for docker
INSTALL (TARGETS qbus_mod_flow DESTINATION docker/flow/mods)

# remove none used ones (needs to fix)
INSTALL (CODE "execute_process(COMMAND docker image rm qbus_mod_flow:1.0.0)")

# build the docker image
INSTALL (CODE "execute_process(COMMAND docker build -t qbus_mod_flow:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")
