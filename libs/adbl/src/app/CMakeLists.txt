#SET(CMAKE_BUILD_TYPE Debug)

INCLUDE_DIRECTORIES("../core")

SET(ADBL_APP_SOURCES
  adbl_app.c
)

add_executable          (adbl_app ${ADBL_APP_SOURCES})
target_link_libraries   (adbl_app adbl2)

#----------------------------------------------------------------------------------

if (PVD_ADBL_DIR)
  add_custom_command (TARGET adbl_app POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_ADBL_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
endif()

#----------------------------------------------------------------------------------
