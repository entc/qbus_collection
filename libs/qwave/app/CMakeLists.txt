
INCLUDE_DIRECTORIES("../src" "../../cape/src")

SET(QWAVE_SERVER_SOURCES
  qwave_server.c
)

add_executable          (qwave_server ${QWAVE_SERVER_SOURCES})
target_link_libraries   (qwave_server qwave)
