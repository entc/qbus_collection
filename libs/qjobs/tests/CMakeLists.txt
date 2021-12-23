INCLUDE_DIRECTORIES("../src" "../../cape/src" "../../adbl/src/core")

add_executable          (ut_qjobs ut_qjobs.c)
target_link_libraries   (ut_qjobs qjobs)

if (ETC_LOCATION)
  add_custom_command (TARGET ut_qjobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

if (PVD_ADBL_DIR)
  add_custom_command (TARGET ut_qjobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_ADBL_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
endif()
