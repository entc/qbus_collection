SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
find_package(Pcre REQUIRED)

INCLUDE_DIRECTORIES(${PCRE_INCLUDE_DIRS} "../../../libs/cape/src" "../../../libs/qbus/src" "../../../libs/adbl/src/core")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_PIPE_SOURCES
  pipe.c
)

SET (QBUS_MODULES_PIPE_HEADERS
)

ADD_EXECUTABLE (qbus_mod_pipe ${QBUS_MODULES_PIPE_SOURCES} ${QBUS_MODULES_PIPE_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_pipe qbus adbl2 ${PCRE_LIBRARIES})

# install
INSTALL (TARGETS qbus_mod_pipe DESTINATION mods)

#----------------------------------------------------------------------------------------

if (ETC_LOCATION)
  add_custom_command (TARGET qbus_mod_pipe POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

if (PVD_ADBL_DIR)
  add_custom_command (TARGET qbus_mod_pipe POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_ADBL_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
endif()

if (PVD_QBUS_DIR)
  add_custom_command (TARGET qbus_mod_pipe POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()

#----------------------------------------------------------------------------------------
