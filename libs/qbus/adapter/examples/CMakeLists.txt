if (TARGET qbus_adapter_python)

  # use this to trigger the make
  add_custom_target (qbus_mod_python_instance ALL COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

  # set some symlinks for direct starting
  add_custom_command (TARGET qbus_mod_python_instance POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/module_instance.py ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
  add_custom_command (TARGET qbus_mod_python_instance POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/qbus_py_instance.sh ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

  # write the bash script
  file (WRITE  "qbus_py_instance.sh" "#!/usr/bin/env bash\n")
  file (APPEND "qbus_py_instance.sh" ${PYTHON_TARGET_EXEC})
  file (APPEND "qbus_py_instance.sh" " module_instance.py\n")

  # set permissions and copy the script to the build folder
  # we cannot finally copy it, because the traget directory is only available at build time
  file (COPY "qbus_py_instance.sh" DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

  # remove the local file
  file (REMOVE "qbus_py_instance.sh")

  if (PVD_QBUS_DIR)
    add_custom_command (TARGET qbus_mod_python_instance POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
  endif()

  if (ETC_LOCATION)
    add_custom_command (TARGET qbus_mod_python_instance POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
  endif()

  if (ADM_QBUS_DIR)
    add_custom_command (TARGET qbus_mod_python_instance POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ADM_QBUS_DIR}/${CMAKE_CFG_INTDIR}/qbus.so ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus.so)
  endif()

  # ---------------------------------------------------------------------------

  # use this to trigger the make
  add_custom_target (qbus_mod_python_background ALL COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

  # set some symlinks for direct starting
  add_custom_command (TARGET qbus_mod_python_background POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/module_background.py ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
  add_custom_command (TARGET qbus_mod_python_background POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/qbus_py_background.sh ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

  # write the bash script
  file (WRITE  "qbus_py_background.sh" "#!/usr/bin/env bash\n")
  file (APPEND "qbus_py_background.sh" ${PYTHON_TARGET_EXEC})
  file (APPEND "qbus_py_background.sh" " module_background.py\n")

  # set permissions and copy the script to the build folder
  # we cannot finally copy it, because the traget directory is only available at build time
  file (COPY "qbus_py_background.sh" DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

  # remove the local file
  file (REMOVE "qbus_py_background.sh")

endif()
