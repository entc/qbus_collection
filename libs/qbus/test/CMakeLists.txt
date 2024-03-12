# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../src")

set(APP_SOURCES
  qbus_test_emu.c
)

set(APP_HEADERS

)

#----------------------------------------------------------------------------------

add_executable          (qbus_test_emu ${APP_SOURCES})
target_link_libraries   (qbus_test_emu qbus_emu)

#----------------------------------------------------------------------------------

if (ETC_LOCATION)
  add_custom_command (TARGET qbus_test_emu POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

if (PVD_QBUS_DIR)
  add_custom_command (TARGET qbus_test_emu POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()
