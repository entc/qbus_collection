INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qbus/src" "../../../libs/qwebs/src" "../../../libs/qcrypt/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_WEBS_SOURCES
  webs.c
  webs_json.c
  webs_post.c
  webs_enjs.c
  webs_imca.c
)

SET (QBUS_MODULES_WEBS_HEADERS
  webs_json.h
  webs_post.h
  webs_enjs.h
  webs_imca.h
)

ADD_EXECUTABLE (qbus_mod_webs ${QBUS_MODULES_WEBS_SOURCES} ${QBUS_MODULES_WEBS_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_webs qwebs qbus qcrypt)

# new install path
INSTALL (TARGETS qbus_mod_webs DESTINATION mods)

# set default symlinks
if (ETC_LOCATION)
  add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/dist ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/dist)
add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/p_site ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/site)
