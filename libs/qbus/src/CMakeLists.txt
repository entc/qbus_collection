# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../../qcrypt/src" "core" "route" "logger" "engines" "protocol" ".")

SET(QBUS_SOURCES
  qbus.c

# include core classes
  core/qbus_config.c
  core/qbus_route.c
  core/qbus_obsvbl.c
  core/qbus_message.c
  core/qbus_chain.c
  core/qbus_methods.c

# include logger classes
  logger/qbus_logger.c
  logger/qbus_logger_file.c
  logger/qbus_logger_udp.c

# include engine classes
  engines/qbus_engines.c
  engines/qbus_engines_pvd.c
)

SET(QBUS_HEADERS
  qbus.h
  qbus.hpp
  qbus_types.h
 
# include core classes
  core/qbus_config.h
  core/qbus_route.h
  core/qbus_obsvbl.h
  core/qbus_message.h
  core/qbus_chain.h
  core/qbus_methods.h

# include logger classes
  logger/qbus_logger.h
  logger/qbus_logger_file.h
  logger/qbus_logger_udp.h

# include engine classes
  engines/qbus_engines.h
  engines/qbus_engines_pvd.h
  engines/qbus_pvd.h
)

#----------------------------------------------------------------------------------

add_library             (qbus SHARED ${QBUS_SOURCES} ${QBUS_HEADERS})
target_link_libraries   (qbus cape qcrypt qbus_frame)
set_target_properties   (qbus PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

#----------------------------------------------------------------------------------

# for local install
INSTALL (TARGETS qbus DESTINATION lib)

#----------------------------------------------------------------------------------

SET (PVD_QBUS_DIR ${CMAKE_CURRENT_BINARY_DIR}/pvd_qbus CACHE PATH "library path to qbus engines")
message (STATUS "PVD_QBUS_DIR  = ${PVD_QBUS_DIR}/${CMAKE_CFG_INTDIR}")

SET (ADM_QBUS_DIR ${CMAKE_CURRENT_BINARY_DIR}/adm_qbus CACHE PATH "library path to qbus adapters")
message (STATUS "ADM_QBUS_DIR  = ${ADM_QBUS_DIR}/${CMAKE_CFG_INTDIR}")

#----------------------------------------------------------------------------------

ADD_LIBRARY (qbus_frame STATIC "core/qbus_frame.c" "core/qbus_frame.h")
TARGET_LINK_LIBRARIES (qbus_frame cape qcrypt)

#----------------------------------------------------------------------------------

set(QBUS_APP_SOURCES
  app/qbus_app.c
)

set(QBUS_APP_HEADERS

)

#----------------------------------------------------------------------------------

add_executable          (qbus_app ${QBUS_APP_SOURCES} ${QBUS_APP_HEADERS})
target_link_libraries   (qbus_app qbus)

#----------------------------------------------------------------------------------------

if (ETC_LOCATION)
  add_custom_command (TARGET qbus_app POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${ETC_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/etc)
endif()

if (PVD_QBUS_DIR)
  add_custom_command (TARGET qbus_app POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PVD_QBUS2_DIR}/${CMAKE_CFG_INTDIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qbus)
endif()

#----------------------------------------------------------------------------------------

SUBDIRS(engines/pvd_tcp)
SUBDIRS(tests)

