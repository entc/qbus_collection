cmake_minimum_required(VERSION 2.4)

INCLUDE_DIRECTORIES("../../libs/cape/src" "../../libs/qbus/src" "../../libs/qwebs/src" "../../libs/qcrypt/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_WEBS_SOURCES
  webs.c
)

SET (QBUS_MODULES_WEBS_HEADERS
)

ADD_EXECUTABLE (qbus_mod_webs ${QBUS_MODULES_WEBS_SOURCES} ${QBUS_MODULES_WEBS_HEADERS})
#TARGET_LINK_LIBRARIES(qbus_mod_webs qwebs qbus qcrypt)
TARGET_LINK_LIBRARIES(qbus_mod_webs cape qwebs qbus qbus_core qbus_engine_tcp qcrypt)

INSTALL (TARGETS qbus_mod_webs DESTINATION modules)

# for docker
#INSTALL (TARGETS qbus_mod_webs DESTINATION docker/webs/mods)
#INSTALL (TARGETS qwebs DESTINATION docker/webs/lib)
#INSTALL (TARGETS qcrypt DESTINATION docker/webs/lib)

# set some symlinks for direct starting
#add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/webs.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/webs.json)

# remove none used ones (needs to fix)
#INSTALL (CODE "execute_process(COMMAND docker image rm qbus_mod_webs:1.0.0)")

# build the docker image
#INSTALL (CODE "execute_process(COMMAND docker build -t qbus_mod_webs:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")
