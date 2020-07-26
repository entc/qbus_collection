cmake_minimum_required(VERSION 2.4)

INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qbus/src" "../../../libs/qwebs/src" "../../../libs/qcrypt/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_WEBS_SOURCES
  webs.c
  webs_json.c
  webs_post.c
)

SET (QBUS_MODULES_WEBS_HEADERS
  webs_json.h
  webs_post.h
)

ADD_EXECUTABLE (qbus_mod_webs ${QBUS_MODULES_WEBS_SOURCES} ${QBUS_MODULES_WEBS_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_webs qwebs qbus qcrypt)
#TARGET_LINK_LIBRARIES(qbus_mod_webs cape qwebs qbus qbus_core qbus_engine_tcp qcrypt)

INSTALL (TARGETS qbus_mod_webs DESTINATION modules)

# set some symlinks for direct starting
add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/webs.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/webs.json)
add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/dist ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/dist)
add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/p_site ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/site)
