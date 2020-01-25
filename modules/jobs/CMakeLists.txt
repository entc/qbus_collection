
INCLUDE_DIRECTORIES(${ENTC_INCLUDE_DIRS} "../../libs/cape/src" "../../libs/qbus/src" "../../libs/adbl/src/core" "../../libs/qcrypt/src" )

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_JOBS_SOURCES
  "jobs.c"
  "jobs_list.c"
  "jobs_sched.c"
)

SET (QBUS_MODULES_JOBS_HEADERS
  "jobs_list.h"
  "jobs_sched.h"
)

ADD_EXECUTABLE (qbus_mod_jobs ${QBUS_MODULES_JOBS_SOURCES} ${QBUS_MODULES_JOBS_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_jobs qbus adbl2 qcrypt)

install (TARGETS qbus_mod_jobs DESTINATION modules)

# for docker
INSTALL (TARGETS qbus_mod_jobs DESTINATION docker/jobs/mods)

# remove none used ones (needs to fix)
INSTALL (CODE "execute_process(COMMAND docker image rm qbus_mod_jobs:1.0.0)")

# build the docker image
INSTALL (CODE "execute_process(COMMAND docker build -t qbus_mod_jobs:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")
