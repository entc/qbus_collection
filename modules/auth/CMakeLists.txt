
INCLUDE_DIRECTORIES("../../libs/cape/src" "../../libs/qbus/src" "../../libs/adbl/src/core" "../../libs/qcrypt/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_AUTH_SOURCES
  "auth.c"
  "auth_tokens.c"
  "auth_ui.c"
  "auth_gp.c"
  "auth_rinfo.c"
)

SET (QBUS_MODULES_AUTH_HEADERS
  "auth_tokens.h"
  "auth_ui.h"
  "auth_gp.h"
  "auth_rinfo.h"
)

ADD_EXECUTABLE (qbus_mod_auth ${QBUS_MODULES_AUTH_SOURCES} ${QBUS_MODULES_AUTH_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_auth qbus adbl2 qcrypt)

INSTALL (TARGETS qbus_mod_auth DESTINATION modules)

# for docker
INSTALL (TARGETS qbus_mod_auth DESTINATION docker/auth/mods)

# remove none used ones (needs to fix)
INSTALL (CODE "execute_process(COMMAND docker image rm qbus_mod_auth:1.0.0)")

# build the docker image
INSTALL (CODE "execute_process(COMMAND docker build -t qbus_mod_auth:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")

