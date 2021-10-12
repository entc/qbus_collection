SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
find_package(PCRE REQUIRED)

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

# set some symlinks for direct starting
add_custom_command (TARGET qbus_mod_pipe POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/pipe.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/pipe.json)
