SUBDIRS (src)
SUBDIRS (app)
SUBDIRS (proto)

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/qwave)
