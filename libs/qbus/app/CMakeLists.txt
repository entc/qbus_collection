INCLUDE_DIRECTORIES("../../cape/src" "../src")

set(APP_SOURCES
  qbus_app.c
)

set(APP_HEADERS

)

#----------------------------------------------------------------------------------

add_executable          (qbus_app ${APP_SOURCES})
target_link_libraries   (qbus_app qbus)
set_target_properties   (qbus_app PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

#----------------------------------------------------------------------------------

if (PVD_QBUS_DIR)
  add_custom_command (TARGET qbus_app POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()

if (ETC_LOCATION)
  add_custom_command (TARGET qbus_app POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()
