# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../cape/src" "..")

set(QBUS_TL2_EMU__SOURCES
  qbus_loader_emu.c
)

set(QBUS_TL2_EMU__HEADERS
  qbus_tl2.h
)

#----------------------------------------------------------------------------------

add_library             (qbus_tl2_emu STATIC ${QBUS_TL2_EMU__SOURCES} ${QBUS_TL2_EMU__HEADERS})
target_link_libraries   (qbus_tl2_emu cape)

#----------------------------------------------------------------------------------
