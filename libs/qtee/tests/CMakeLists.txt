# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../src")
SET(CMAKE_BUILD_TYPE Release)                                   # build type

add_executable          (qtee_ut_template qtee_ut_template.c)
target_link_libraries   (qtee_ut_template qtee cape)

add_executable          (qtee_ut_eval qtee_ut_eval.c)
target_link_libraries   (qtee_ut_eval qtee cape)
