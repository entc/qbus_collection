INCLUDE_DIRECTORIES("../src" "../../cape/src" "../../adbl/src/core")

add_executable          (ut_qjobs ut_qjobs.c)
target_link_libraries   (ut_qjobs qjobs)

add_custom_command (TARGET ut_qjobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/etc/adbl_default.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl_default.json)
add_custom_command (TARGET ut_qjobs POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{ADBL_MYSQL_BINDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/adbl)
