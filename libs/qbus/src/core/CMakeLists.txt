# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../src")

set(CORE_SOURCES
  qbus_engine.c
  qbus_connection.c
  qbus_frame.c
  qbus_route.c
  qbus_route_items.c
  qbus_route_method.c
  qbus_logger.c
)

set(CORE_HEADERS
  qbus_engine.h
  qbus_connection.h
  qbus_frame.h
  qbus_route.h
  qbus_route_items.h
  qbus_route_method.h
  qbus_logger.h
)

#----------------------------------------------------------------------------------

add_library             (qbus_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_link_libraries   (qbus_core cape)
set_target_properties   (qbus_core PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

#----------------------------------------------------------------------------------


