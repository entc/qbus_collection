if (TARGET qbus_adapter_python)

  # use this to trigger the make
  add_custom_target (qbus_mod_python ALL COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

  # install the python file
  install (FILES module.py DESTINATION mods)

  # set some symlinks for direct starting
  add_custom_command (TARGET qbus_mod_python POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/module.py ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

  # write the bash script
  file (WRITE  "qbus_mod_python.sh" "#!/usr/bin/env bash\n")
  file (APPEND "qbus_mod_python.sh" ${PYTHON_TARGET_EXEC})
  file (APPEND "qbus_mod_python.sh" " module.py\n")

  # set permissions and copy the script to the build folder
  file (COPY "qbus_mod_python.sh" DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

  # remove the local file
  file (REMOVE "qbus_mod_python.sh")

  add_custom_command (TARGET qbus_mod_python POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/qbus_mod_python.sh ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

  if (ETC_LOCATION)
    add_custom_command (TARGET qbus_mod_python POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
  endif()

  if (ADM_QBUS_DIR)
    add_custom_command (TARGET qbus_mod_python POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ADM_QBUS_DIR}/${CMAKE_CFG_INTDIR}/qbus.so ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus.so)
  endif()

endif()

