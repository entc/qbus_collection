
INCLUDE_DIRECTORIES("../../l_3rdp/cape/src" "../../l_3rdp/qbus/src" "../../l_3rdp/qwebs/src" "../../l_3rdp/qcrypt/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_WEBS_SOURCES
  webs.c
)

SET (QBUS_MODULES_WEBS_HEADERS
)

ADD_EXECUTABLE (qbus_mod_webs ${QBUS_MODULES_WEBS_SOURCES} ${QBUS_MODULES_WEBS_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_webs qwebs qbus qcrypt)

INSTALL (TARGETS qbus_mod_webs DESTINATION modules)

# for docker
INSTALL (TARGETS qbus_mod_webs DESTINATION docker/webs/mods)
INSTALL (TARGETS qwebs DESTINATION docker/webs/lib)
INSTALL (TARGETS qcrypt DESTINATION docker/webs/lib)

# remove none used ones (needs to fix)
INSTALL (CODE "execute_process(COMMAND docker image rm qbus_mod_webs:1.0.0)")

# build the docker image
INSTALL (CODE "execute_process(COMMAND docker build -t qbus_mod_webs:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")
