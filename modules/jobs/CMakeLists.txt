
INCLUDE_DIRECTORIES(${ENTC_INCLUDE_DIRS} "../../libs/cape/src" "../../libs/qbus/src" "../../libs/adbl/src/core" "../../libs/qcrypt/src" "../../libs/qjobs/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_JOBS_SOURCES
  "jobs.c"
  "jobs_list.c"
)

SET (QBUS_MODULES_JOBS_HEADERS
  "jobs_list.h"
)

ADD_EXECUTABLE (qbus_mod_jobs ${QBUS_MODULES_JOBS_SOURCES} ${QBUS_MODULES_JOBS_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_jobs qbus adbl2 qcrypt qjobs)

install (TARGETS qbus_mod_jobs DESTINATION modules)

# set some symlinks for direct starting
add_custom_command (TARGET qbus_mod_jobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/jobs.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/jobs.json)
add_custom_command (TARGET qbus_mod_jobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/adbl_default.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl_default.json)
add_custom_command (TARGET qbus_mod_jobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{ADBL_MYSQL_BINDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)

# for docker
INSTALL (TARGETS qbus_mod_jobs DESTINATION docker/jobs/mods)

# remove none used ones (needs to fix)
INSTALL (CODE "execute_process(COMMAND docker image rm qbus_mod_jobs:1.0.0)")

# build the docker image
INSTALL (CODE "execute_process(COMMAND docker build -t qbus_mod_jobs:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")
