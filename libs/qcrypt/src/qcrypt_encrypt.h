#ifndef __QCRYPT_ENCRYPT__H
#define __QCRYPT_ENCRYPT__H 1

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

struct QEncryptBase64_s; typedef struct QEncryptBase64_s* QEncryptBase64;

__CAPE_LIBEX  QEncryptBase64  qencrypt_base64_new       (CapeStream r_product);

__CAPE_LIBEX  void            qencrypt_base64_del       (QEncryptBase64*);

__CAPE_LIBEX  int             qencrypt_base64_process   (QEncryptBase64, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  int             qencrypt_base64_finalize  (QEncryptBase64, CapeErr err);

//-----------------------------------------------------------------------------

struct QEncryptAES_s; typedef struct QEncryptAES_s* QEncryptAES;

__CAPE_LIBEX  QEncryptAES     qencrypt_aes_new          (CapeStream r_product, number_t cypher_type, number_t padding_type, const CapeString secret, number_t key_type);

__CAPE_LIBEX  void            qencrypt_aes_del          (QEncryptAES*);

__CAPE_LIBEX  int             qencrypt_aes_process      (QEncryptAES, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  int             qencrypt_aes_finalize     (QEncryptAES, CapeErr err);

//-----------------------------------------------------------------------------

#endif
