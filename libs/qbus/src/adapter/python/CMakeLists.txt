SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package (PythonTarget)

IF(PYTHON_TARGET_FOUND)

  # abstract operation-system layer
  INCLUDE_DIRECTORIES("../../../cape/src" "../.." ${PYTHON_TARGET_INCLUDES})

  set(ADAPTER_PYTHON_SOURCES
    py_adapter.c
    py_tools.c
    py_qbus.c
  )

  set(ADAPTER_PYTHON_HEADERS
    py_tools.h
    py_qbus.h
  )

  add_library             (qbus_adapter_python SHARED ${ADAPTER_PYTHON_SOURCES} ${ADAPTER_PYTHON_HEADERS})
  target_link_libraries   (qbus_adapter_python cape qbus ${PYTHON_TARGET_LIBRARIES})

  # python loads a module always without the lib prefix and with the suffix
  # regardless on which platform it runs
  # -> default settings will be overidden
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${ADM_QBUS_DIR})
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES OUTPUT_NAME qbus)
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES SUFFIX .so)
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES PREFIX "")

  install (TARGETS qbus_adapter_python DESTINATION mods)

ENDIF()
