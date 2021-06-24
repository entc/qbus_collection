INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qbus/src" "../../../libs/qwebs/src" "../../../libs/qcrypt/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_WEBS_SOURCES
  webs.c
  webs_json.c
  webs_post.c
  webs_imca.c
)

SET (QBUS_MODULES_WEBS_HEADERS
  webs_json.h
  webs_post.h
  webs_imca.h
)

ADD_EXECUTABLE (qbus_mod_webs ${QBUS_MODULES_WEBS_SOURCES} ${QBUS_MODULES_WEBS_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_webs qwebs qbus qcrypt)

# new install path
INSTALL (TARGETS qbus_mod_webs DESTINATION mods)



  if(DEFINED ENV{QBUS_ETC_DIR})
    set(ETCDIR ENV{QBUS_ETC_DIR})
  else()
    set(ETCDIR "base")    
  endif()

  message (STATUS "QBUS_ETC_DIR  = ${PROJECT_SOURCE_DIR}/etc/${ETCDIR}")
  message (STATUS "BIN_DIR = ${CMAKE_CURRENT_BINARY_DIR}")
  
  # set default symlinks
  IF(EXISTS ${PROJECT_SOURCE_DIR}/etc/${ETCDIR})
    add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/${ETCDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
    add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{ADBL_MYSQL_BINDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
  ENDIF()

add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/dist ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/dist)
add_custom_command (TARGET qbus_mod_webs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/p_site ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/site)
