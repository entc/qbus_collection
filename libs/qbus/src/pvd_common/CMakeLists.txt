INCLUDE_DIRECTORIES(../lib ${PROJECT_SOURCE_DIR}/qbus_collection/libs/cape/src ${PROJECT_SOURCE_DIR}/qbus_collection/libs/qcrypt/src)

SET (PVD_COMMON_SOURCES
  qbus_frame.c
)

SET (PVD_COMMON_HEADERS
  qbus_pvd.h
  qbus_frame.h
)

ADD_LIBRARY (qbus_pvd_common STATIC ${PVD_COMMON_SOURCES} ${PVD_COMMON_HEADERS})
TARGET_LINK_LIBRARIES (qbus_pvd_common cape qcrypt)
