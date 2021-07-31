# abstract operation-system layer
INCLUDE_DIRECTORIES("..")

add_executable          (ut_qbus_local ut_qbus_local.c)
target_link_libraries   (ut_qbus_local qbus)

