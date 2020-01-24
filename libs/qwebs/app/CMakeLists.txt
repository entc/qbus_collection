
INCLUDE_DIRECTORIES("../src" "../../cape/src")

SET(QWEBS_APP_SOURCES
  qwebs_app.c
)

add_executable          (qwebs_app ${QWEBS_APP_SOURCES})
target_link_libraries   (qwebs_app qwebs)
