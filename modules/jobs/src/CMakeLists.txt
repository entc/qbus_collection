
INCLUDE_DIRECTORIES(${ENTC_INCLUDE_DIRS} "../../../libs/cape/src" "../../../libs/qbus/src" "../../../libs/adbl/src/core" "../../../libs/qcrypt/src" "../../../libs/qjobs/src")

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

INSTALL (TARGETS qbus_mod_jobs DESTINATION mods)

# set default symlinks

if (ETC_LOCATION)
  add_custom_command (TARGET qbus_mod_jobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

if (PVD_ADBL_DIR)
  add_custom_command (TARGET qbus_mod_jobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_ADBL_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
endif()

if (PVD_QBUS_DIR)
  add_custom_command (TARGET qbus_mod_jobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()
