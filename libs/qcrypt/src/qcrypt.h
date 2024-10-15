#ifndef __QCRYPT__H
#define __QCRYPT__H 1

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "sys/cape_file.h"
#include "stc/cape_str.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX  CapeString     qcrypt__decrypt            (const CapeString vsec, const CapeString encrypted_text, CapeErr err);

__CAPE_LIBEX  int            qcrypt__decrypt_node       (const CapeString vsec, const CapeString encrypted_text, CapeUdc dest_list, CapeErr err);

__CAPE_LIBEX  int            qcrypt__decrypt_row_text   (const CapeString vsec, const CapeUdc row, const CapeString data_name, CapeErr err);

__CAPE_LIBEX  int            qcrypt__decrypt_row_node   (const CapeString vsec, const CapeUdc row, const CapeString data_name, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  CapeString     qcrypt__encrypt            (const CapeString vsec, const CapeString decrypted_text, CapeErr err);

//-----------------------------------------------------------------------------
// encode / decode methods

__CAPE_LIBEX  CapeString     qcrypt__encode_base64_m     (const CapeStream source);

__CAPE_LIBEX  CapeString     qcrypt__encode_base64_o     (const char* bufdat, number_t buflen, const CapeString prefix);

__CAPE_LIBEX  CapeStream     qcrypt__decode_base64_s     (const CapeString source);

__CAPE_LIBEX  CapeStream     qcrypt__decode_base64_o     (const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------
// hash methods

__CAPE_LIBEX  CapeString     qcrypt__hash_sha512__hex_m  (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeString     qcrypt__hash_sha512__hex_o  (const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_sha512__bin_m  (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_sha512__bin_o  (const char* bufdat, number_t buflen, CapeErr err);

//-----------------------------------------------------------------------------
// hash methods

__CAPE_LIBEX  CapeString     qcrypt__hash_sha256__hex_m  (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeString     qcrypt__hash_sha256__hex_o  (const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_sha256__bin_m  (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_sha256__bin_o  (const char* bufdat, number_t buflen, CapeErr err);

//-----------------------------------------------------------------------------
// don't use this as security feature, sha-1 is broken and unsafe

__CAPE_LIBEX  CapeString     qcrypt__hash_sha__hex_m     (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeString     qcrypt__hash_sha__hex_o     (const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_sha__bin_m     (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_sha__bin_o     (const char* bufdat, number_t buflen, CapeErr err);

//-----------------------------------------------------------------------------
// don't use this as security feature, md5 is broken and unsafe

__CAPE_LIBEX  CapeString     qcrypt__hash_md5__hex_m     (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeString     qcrypt__hash_md5__hex_o     (const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_md5__bin_m     (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_md5__bin_o     (const char* bufdat, number_t buflen, CapeErr err);

//-----------------------------------------------------------------------------
// definitions for cape stream encode/decode

__CAPE_LIBEX  CapeString __STDCALL qcrypt__stream_base64_encode (const CapeStream s);
__CAPE_LIBEX  CapeStream __STDCALL qcrypt__stream_base64_decode (const CapeString);

//-----------------------------------------------------------------------------

#endif
