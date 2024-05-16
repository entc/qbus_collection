# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../cape/src" "..")

set(QBUS_TL1_HEADERS
  qbus_tl1.h
  qbus_chains.h
  qbus_member.h
  qbus_subscriber.h
)

set(QBUS_TL1_SOURCES
  qbus_chains.c
  qbus_member.c
  qbus_subscriber.c
)

#----------------------------------------------------------------------------------

add_library             (qbus_tl1_emu STATIC qbus_tl1_emu.c ${QBUS_TL1_SOURCES} ${QBUS_TL1_HEADERS})
target_link_libraries   (qbus_tl1_emu cape)

#----------------------------------------------------------------------------------

add_library             (qbus_tl1_emi STATIC qbus_tl1_lib.c ${QBUS_TL1_SOURCES} ${QBUS_TL1_HEADERS})
target_link_libraries   (qbus_tl1_emi qbus_tl2_emu cape)

#----------------------------------------------------------------------------------

add_library             (qbus_tl1 STATIC qbus_tl1_lib.c ${QBUS_TL1_SOURCES} ${QBUS_TL1_HEADERS})
target_link_libraries   (qbus_tl1 qbus_tl2_lib cape)

#----------------------------------------------------------------------------------
