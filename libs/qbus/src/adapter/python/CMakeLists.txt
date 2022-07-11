
find_package (Python3 COMPONENTS Interpreter Development QUIET)

IF(Python3_FOUND)

  message (STATUS "Python3_INCLUDE_DIRS  = ${Python3_INCLUDE_DIRS}")
  message (STATUS "Python3_LIBRARIES = ${Python3_LIBRARIES}")
  message (STATUS "Python3_VERSION = ${Python3_VERSION}")

  # abstract operation-system layer
  INCLUDE_DIRECTORIES("../../../cape/src" "../../src" ${Python3_INCLUDE_DIRS})

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
  target_link_libraries   (qbus_adapter_python cape qbus ${Python3_LIBRARIES})

  # python loads a module always without the lib prefix and with the suffix
  # regardless on which platform it runs
  # -> default settings will be overidden
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${ADM_QBUS_DIR})
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES OUTPUT_NAME qbus)
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES SUFFIX .so)
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES PREFIX "")

ELSE()

find_package (Python2 COMPONENTS Interpreter Development QUIET)

IF(Python2_FOUND)

  message (STATUS "Python2_INCLUDE_DIRS  = ${Python2_INCLUDE_DIRS}")
  message (STATUS "Python2_LIBRARIES = ${Python2_LIBRARIES}")
  message (STATUS "Python2_VERSION = ${Python2_VERSION}")

ELSE()

find_package (PythonLibs)

IF(PYTHONLIBS_FOUND)

  message (STATUS "PythonLibs_INCLUDE_DIRS  = ${PYTHON_INCLUDE_DIRS}")
  message (STATUS "PythonLibs_LIBRARIES = ${PYTHON_LIBRARIES}")
  message (STATUS "PythonLibs_VERSION = ${PYTHONLIBS_VERSION_STRING}")

    # abstract operation-system layer
  INCLUDE_DIRECTORIES("../../../cape/src" "../../src" ${PYTHON_INCLUDE_DIRS})

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
  target_link_libraries   (qbus_adapter_python cape qbus ${PYTHON_LIBRARIES})

  # python loads a module always without the lib prefix and with the suffix
  # regardless on which platform it runs
  # -> default settings will be overidden
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${ADM_QBUS_DIR})
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES OUTPUT_NAME qbus)
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES SUFFIX .so)
  SET_TARGET_PROPERTIES   (qbus_adapter_python PROPERTIES PREFIX "")

ENDIF()
  
ENDIF()

ENDIF()



