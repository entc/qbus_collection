#ifndef __QCRYPT_DECRYPT__H
#define __QCRYPT_DECRYPT__H 1

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_stream.h"

#include "qcrypt_aes.h"

//-----------------------------------------------------------------------------

struct QDecryptBase64_s; typedef struct QDecryptBase64_s* QDecryptBase64;

__CAPE_LIBEX  QDecryptBase64  qdecrypt_base64_new       (CapeStream r_product);

__CAPE_LIBEX  void            qdecrypt_base64_del       (QDecryptBase64*);

__CAPE_LIBEX  int             qdecrypt_base64_process   (QDecryptBase64, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  int             qdecrypt_base64_finalize  (QDecryptBase64, CapeErr err);

//-----------------------------------------------------------------------------

struct QDecryptAES_s; typedef struct QDecryptAES_s* QDecryptAES;

__CAPE_LIBEX  QDecryptAES     qdecrypt_aes_new          (CapeStream r_product, number_t cypher_type, number_t padding_type, const CapeString secret, number_t key_type);

__CAPE_LIBEX  void            qdecrypt_aes_del          (QDecryptAES*);

__CAPE_LIBEX  int             qdecrypt_aes_process      (QDecryptAES, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  int             qdecrypt_aes_finalize     (QDecryptAES, CapeErr err);

//-----------------------------------------------------------------------------

#endif
