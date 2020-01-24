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

struct QDecryptAES_s; typedef struct QDecryptAES_s* QDecryptAES;

__CAPE_LIBEX  QDecryptAES     qdecrypt_aes_new          (CapeStream r_product, const CapeString secret, number_t cypher_type, number_t key_type);

__CAPE_LIBEX  void            qdecrypt_aes_del          (QDecryptAES*);

__CAPE_LIBEX  int             qdecrypt_aes_process      (QDecryptAES, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  int             qdecrypt_aes_finalize     (QDecryptAES, CapeErr err);

//-----------------------------------------------------------------------------

#endif
