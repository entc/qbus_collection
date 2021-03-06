INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qbus/src" "../../../libs/adbl/src/core" "../../../libs/qcrypt/src")

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
)

ADD_EXECUTABLE (qbus_mod_auth ${QBUS_MODULES_AUTH_SOURCES} ${QBUS_MODULES_AUTH_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_auth qbus adbl2 qcrypt)

INSTALL (TARGETS qbus_mod_auth DESTINATION mods)

if(DEFINED ENV{QBUS_ETC_DIR})
  set(ETCDIR $ENV{QBUS_ETC_DIR})
else()
  set(ETCDIR "base")    
endif()

message (STATUS "QBUS_ETC_DIR  = ${PROJECT_SOURCE_DIR}/etc/${ETCDIR}")
message (STATUS "BIN_DIR = ${CMAKE_CURRENT_BINARY_DIR}")

# set some symlinks for direct starting
add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/${ETCDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{ADBL_MYSQL_BINDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
