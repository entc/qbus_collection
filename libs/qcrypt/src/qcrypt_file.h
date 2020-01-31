#ifndef __QCRYPT_FILE_H
#define __QCRYPT_FILE_H 1

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "sys/cape_file.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX  int            qcrypt_decrypt_file        (const CapeString vsec, const CapeString file, void* ptr, fct_cape_fs_file_load, CapeErr err);

//-----------------------------------------------------------------------------
// decrypt using a file

/*
struct QCryptDecrypt_s; typedef struct QCryptDecrypt_s* QCryptDecrypt;

__CAPE_LIBEX  QCryptDecrypt  qcrypt_decrypt_new         (const CapeString path, const CapeString file, const CapeString vsec);

__CAPE_LIBEX  void           qcrypt_decrypt_del         (QCryptDecrypt*);

__CAPE_LIBEX  int            qcrypt_decrypt_open        (QCryptDecrypt, CapeErr err);

__CAPE_LIBEX  number_t       qcrypt_decrypt_next        (QCryptDecrypt, char* buffer, number_t len);

//-----------------------------------------------------------------------------
// encrypt using a file

struct QCryptFile_s; typedef struct QCryptFile_s* QCryptFile;

__CAPE_LIBEX  QCryptFile     qcrypt_file_new            (const CapeString file);

__CAPE_LIBEX  void           qcrypt_file_del            (QCryptFile*);

__CAPE_LIBEX  int            qcrypt_file_encrypt        (QCryptFile, const CapeString vsec, CapeErr err);

__CAPE_LIBEX  int            qcrypt_file_write          (QCryptFile, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  int            qcrypt_file_finalize       (QCryptFile, CapeErr err);

//-----------------------------------------------------------------------------
*/

#endif
