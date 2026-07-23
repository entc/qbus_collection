INCLUDE_DIRECTORIES("../../../qbus_collection/libs/cape/src" "../../../qbus_collection/libs/qcrypt/src")

set(QDATA_SOURCES
  qdata.c
)

set(QDATA_HEADERS
  qdata.h
)

add_library             (qdata STATIC ${QDATA_SOURCES} ${QDATA_HEADERS})
target_link_libraries   (qdata cape qcrypt)
