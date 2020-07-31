SUBDIRS (src)

# for docker install
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/cape)
INSTALL (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake DESTINATION ${CMAKE_BINARY_DIR}/docker/qbus_collection/libs/cape)
