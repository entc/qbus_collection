cmake_minimum_required(VERSION 2.4)

PROJECT(qcrypt)

SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

find_package(Crypt REQUIRED)
#find_package(Entc_1_4 REQUIRED)

#----------------------------------------------------------------------------------

SET(QCRYPT_CORE_SOURCES
  qcrypt.c
  qcrypt_aes.c
  qcrypt_encrypt.c
  qcrypt_decrypt.c
  qcrypt_file.c
)

SET(QCRYPT_CORE_HEADERS
  qcrypt.h
  qcrypt_aes.h
  qcrypt_encrypt.h
  qcrypt_decrypt.h
  qcrypt_file.h
)

#----------------------------------------------------------------------------------

INCLUDE_DIRECTORIES("." "../../cape/src" ${CRYPT_INCLUDES} ${ENTC_INCLUDE_DIRS})

add_library             (qcrypt SHARED ${QCRYPT_CORE_SOURCES} ${QCRYPT_CORE_HEADERS})
target_link_libraries   (qcrypt cape ${CRYPT_LIBRARIES} ${ENTC_LIBRARIES})

install (TARGETS qcrypt DESTINATION lib)

# for docker
INSTALL (TARGETS qcrypt DESTINATION docker/qore/lib)

#----------------------------------------------------------------------------------


