# abstract operation-system layer
INCLUDE_DIRECTORIES("..")

add_executable          (ut_qbus_local ut_qbus_local.c)
target_link_libraries   (ut_qbus_local qbus)

add_executable          (ut_qbus_perform ut_qbus_perform.c)
target_link_libraries   (ut_qbus_perform qbus)

if (PVD_QBUS_DIR)
  add_custom_command (TARGET ut_qbus_perform POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()
