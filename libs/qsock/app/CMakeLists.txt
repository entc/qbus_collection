
INCLUDE_DIRECTORIES("../src" "../../cape/src")

SET(QWAVE_SERVER_SOURCES
  qsock_client_app.c
)

add_executable          (qsock_client ${QWAVE_SERVER_SOURCES})
target_link_libraries   (qsock_client qsock)
