INCLUDE_DIRECTORIES("../../cape/src")

#----------------------------------------------------------------------------------

SET(QTEE_SOURCES
  qtee_template.c
  qtee_eval.c
  qtee_format.c
  qtee_gen.c
)

SET(QTEE_HEADERS
  qtee_template.h
  qtee_eval.h
  qtee_format.h
  qtee_gen.h
)

#----------------------------------------------------------------------------------

add_library             (qtee STATIC ${QTEE_SOURCES} ${QTEE_HEADERS})
target_link_libraries   (qtee cape)

install (TARGETS qtee DESTINATION lib)

#----------------------------------------------------------------------------------
