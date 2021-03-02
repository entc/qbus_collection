cmake_minimum_required(VERSION 2.4)

INCLUDE_DIRECTORIES("../../../libs/cape/src" "../../../libs/qbus/src")

#----------------------------------------------------------------------------------------

SET (QBUS_MODULES_PIPE_SOURCES
  pipe.c
)

SET (QBUS_MODULES_PIPE_HEADERS
)

ADD_EXECUTABLE (qbus_mod_pipe ${QBUS_MODULES_PIPE_SOURCES} ${QBUS_MODULES_PIPE_HEADERS})
TARGET_LINK_LIBRARIES(qbus_mod_pipe qbus)

# install
INSTALL (TARGETS qbus_mod_pipe DESTINATION mods)

# set some symlinks for direct starting
add_custom_command (TARGET qbus_mod_pipe POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/pipe.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/pipe.json)
