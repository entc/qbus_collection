
INCLUDE_DIRECTORIES("../src" "../../cape/src")

SET(QWAVE_SERVER_SOURCES
  qwave_proto.c
)

add_executable          (qwave_proto ${QWAVE_SERVER_SOURCES})
target_link_libraries   (qwave_proto qwave)
