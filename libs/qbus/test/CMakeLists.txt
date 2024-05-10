# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../src")

#----------------------------------------------------------------------------------

add_executable          (qbus_test1_emu "qbus_test1_emu.c")
target_link_libraries   (qbus_test1_emu qbus_emu)

#----------------------------------------------------------------------------------

if (ETC_LOCATION)
  add_custom_command (TARGET qbus_test1_emu POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

if (PVD_QBUS_DIR)
  add_custom_command (TARGET qbus_test1_emu POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()
