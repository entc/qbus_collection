
INCLUDE_DIRECTORIES("../src" "../../cape/src")

SET(QWAVE_SERVER_SOURCES
  qwave_server.c
)

add_executable          (qwave_server ${QWAVE_SERVER_SOURCES})
target_link_libraries   (qwave_server qwave)

add_custom_command (TARGET qwave_server POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/*.html ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
