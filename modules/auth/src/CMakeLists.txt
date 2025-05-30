INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qbus/src" "../../../libs/adbl/src/core" "../../../libs/qcrypt/src" "../../../libs/qjobs/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_AUTH_SOURCES
  "auth.c"
  "auth_vault.c"
  "auth_tokens.c"
  "auth_ui.c"
  "auth_gp.c"
  "auth_rinfo.c"
  "auth_perm.c"
  "auth_session.c"
  "auth_msgs.c"
  "auth_roles.c"
)

SET (QBUS_MODULES_AUTH_HEADERS
  "auth_vault.h"
  "auth_tokens.h"
  "auth_ui.h"
  "auth_gp.h"
  "auth_rinfo.h"
  "auth_perm.h"
  "auth_session.h"
  "auth_msgs.h"
  "auth_roles.h"
)

ADD_EXECUTABLE (qbus_mod_auth ${QBUS_MODULES_AUTH_SOURCES} ${QBUS_MODULES_AUTH_HEADERS})
TARGET_LINK_LIBRARIES (qbus_mod_auth qbus adbl2 qcrypt qjobs)

INSTALL (TARGETS qbus_mod_auth DESTINATION mods)
add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/*.sh ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

# set default symlinks

if (ETC_LOCATION)
  add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

if (PVD_ADBL_DIR)
  add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_ADBL_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
endif()

if (PVD_QBUS_DIR)
  add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()
