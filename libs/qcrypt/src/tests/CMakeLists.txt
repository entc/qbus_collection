INCLUDE_DIRECTORIES("..")

add_executable          (ut_qcrypt_file ut_qcrypt_file.c)
target_link_libraries   (ut_qcrypt_file qcrypt)

add_executable          (ut_qcrypt_enc ut_qcrypt_enc.c)
target_link_libraries   (ut_qcrypt_enc qcrypt)

