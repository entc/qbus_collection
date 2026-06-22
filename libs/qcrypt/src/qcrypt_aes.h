#ifndef __QCRYPT_AES__H
#define __QCRYPT_AES__H 1

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

// paddings for keys
#define QCRYPT_PADDING_NONE                    0x00
#define QCRYPT_PADDING_ANSI_X923               0x01
#define QCRYPT_PADDING_ZEROS                   0x02
#define QCRYPT_PADDING_PKCS7                   0x03

// other options
#define QCRYPT_KEY_NONE                        0x01
#define QCRYPT_KEY_PASSPHRASE_MD5              0x04     // default for passphrase

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

struct QCryptAESKeys_s; typedef struct QCryptAESKeys_s* QCryptAESKeys;

// openssl types
struct evp_cipher_st;
struct evp_cipher_ctx_st;

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__sha256         (const CapeString secret, const struct evp_cipher_st* cypher, CapeErr err);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__md5_en         (const CapeString secret, const struct evp_cipher_st* cypher, CapeStream product);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__md5_de         (const CapeString secret, const struct evp_cipher_st* cypher, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__padding_zero   (const CapeString secret, const struct evp_cipher_st* cypher);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__ansiX923       (const CapeString secret, const struct evp_cipher_st* cypher);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__padding_pkcs7  (const CapeString secret, const struct evp_cipher_st* cypher);

__CAPE_LIBEX  QCryptAESKeys      qcrypt_aes_keys_new__pbkdf2         (const CapeString secret, number_t iv_len, number_t salt_len, CapeErr err);

__CAPE_LIBEX  void               qcrypt_aes_keys_del                 (QCryptAESKeys* p_self);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  const CapeString   qcrypt_aes_key                      (QCryptAESKeys);

__CAPE_LIBEX  const CapeString   qcrypt_aes_iv                       (QCryptAESKeys);

__CAPE_LIBEX  const CapeString   qcrypt_aes_salt                     (QCryptAESKeys);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  void               qcrypt_aes_padding__ansiX923_pad    (unsigned char* bufdat, number_t buflen, number_t offset);

__CAPE_LIBEX  int                qcrypt_aes__handle_error            (struct evp_cipher_ctx_st* ctx, CapeErr err);

//-----------------------------------------------------------------------------

#endif

#endif
