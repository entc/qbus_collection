#ifndef __QCRYPT_AES__H
#define __QCRYPT_AES__H 1

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_stream.h"

// openssl includes
#include <openssl/evp.h>

//-----------------------------------------------------------------------------

#define QCRYPT_AES_TYPE_CBC                0x01
#define QCRYPT_AES_TYPE_CFB                0x02

#define QCRYPT_AES_TYPE_CFB_1              0xF2
#define QCRYPT_AES_TYPE_CFB_8              0xF3
#define QCRYPT_AES_TYPE_CFB_128            0xF4

#define QCRYPT_KEY_SHA256                  0x00     // default

// paddings for keys
#define QCRYPT_PADDING_ANSI_X923           0x01
#define QCRYPT_PADDING_ZEROS               0x02
#define QCRYPT_PADDING_PKCS7               0x03

// other options
#define QCRYPT_KEY_PASSPHRASE_MD5          0x04     // default for passphrase

//-----------------------------------------------------------------------------

struct QCryptAESKeys_s; typedef struct QCryptAESKeys_s* QCryptAESKeys;

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__sha256         (const CapeString secret, const EVP_CIPHER* cypher, CapeErr err);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__md5_en         (const CapeString secret, const EVP_CIPHER* cypher, CapeStream product);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__md5_de         (const CapeString secret, const EVP_CIPHER* cypher, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__padding_zero   (const CapeString secret, const EVP_CIPHER* cypher);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__ansiX923       (const CapeString secret, const EVP_CIPHER* cypher);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__padding_pkcs7  (const CapeString secret, const EVP_CIPHER* cypher);

__CAPE_LIBEX  void               qcrypt_aes_keys_del                 (QCryptAESKeys* p_self);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  unsigned char*     qcrypt_aes_key                      (QCryptAESKeys);

__CAPE_LIBEX  unsigned char*     qcrypt_aes_iv                       (QCryptAESKeys);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  void               qcrypt_aes_padding__ansiX923_pad    (unsigned char* bufdat, number_t buflen, number_t offset);

__CAPE_LIBEX  int                qcrypt_aes__handle_error            (EVP_CIPHER_CTX* ctx, CapeErr err);

__CAPE_LIBEX  const EVP_CIPHER*  qencrypt_aes__get_cipher            (number_t type);

//-----------------------------------------------------------------------------

#endif
