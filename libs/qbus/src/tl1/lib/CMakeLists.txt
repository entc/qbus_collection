# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../../cape/src" "..")

set(QBUS_TL1_SOURCES
  qbus_manifold.c
)

set(QBUS_TL1_HEADERS
  ../qbus_tl1.h
)

#----------------------------------------------------------------------------------

add_library             (qbus_tl1 STATIC ${QBUS_TL1_SOURCES} ${QBUS_TL1_HEADERS})
target_link_libraries   (qbus_tl1 cape)

#----------------------------------------------------------------------------------
