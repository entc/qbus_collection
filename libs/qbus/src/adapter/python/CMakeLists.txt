SET(CMAKE_SKIP_RPATH TRUE)

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
  set_target_properties   (qbus_adapter_python PROPERTIES OUTPUT_NAME qbus)
  set_target_properties   (qbus_adapter_python PROPERTIES SUFFIX .so)
  set_target_properties   (qbus_adapter_python PROPERTIES PREFIX "")

ENDIF()
