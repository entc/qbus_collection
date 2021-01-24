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
)

SET (QBUS_MODULES_AUTH_HEADERS
  "auth_vault.h"
  "auth_tokens.h"
  "auth_ui.h"
  "auth_gp.h"
  "auth_rinfo.h"
  "auth_perm.h"
)

ADD_EXECUTABLE (qbus_mod_auth ${QBUS_MODULES_AUTH_SOURCES} ${QBUS_MODULES_AUTH_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_auth qbus adbl2 qcrypt ${ENTC_LIBRARIES})

INSTALL (TARGETS qbus_mod_auth DESTINATION mods)

# set some symlinks for direct starting
add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/auth.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/auth.json)
add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/adbl_default.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl_default.json)
add_custom_command (TARGET qbus_mod_auth POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{ADBL_MYSQL_BINDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)

# for docker
INSTALL (TARGETS qbus_mod_auth DESTINATION docker/auth/mods)
INSTALL (FILES ${ENTC_LIBRARIES} DESTINATION docker/auth/lib)
#INSTALL (TARGETS cape DESTINATION docker/wspc/lib)
#INSTALL (TARGETS qcrypt DESTINATION docker/wspc/lib)
#INSTALL (TARGETS qbus DESTINATION docker/wspc/lib)
#INSTALL (TARGETS adbl2 DESTINATION docker/wspc/lib)

# remove none used ones (needs to fix)
#INSTALL (CODE "execute_process(COMMAND docker image rm qbus_mod_auth:1.0.0)")

# build the docker image
#INSTALL (CODE "execute_process(COMMAND docker build -t qbus_mod_auth:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")

