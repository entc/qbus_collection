# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../cape/src" "..")

set(QBUS_TL2_EMU__HEADERS
  qbus_tl2.h
)

#----------------------------------------------------------------------------------

add_library             (qbus_tl2_emu STATIC qbus_loader_emu.c ${QBUS_TL2_EMU__HEADERS})
target_link_libraries   (qbus_tl2_emu cape)

#----------------------------------------------------------------------------------

add_library             (qbus_tl2_lib STATIC qbus_loader_lib.c ${QBUS_TL2_EMU__HEADERS})
target_link_libraries   (qbus_tl2_lib cape)

#----------------------------------------------------------------------------------
