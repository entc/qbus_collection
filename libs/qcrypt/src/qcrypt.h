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

__CAPE_LIBEX  CapeString     qcrypt__base64__encrypt    (const CapeString source);

//-----------------------------------------------------------------------------
// encode / decode methods

__CAPE_LIBEX  CapeString     qcrypt__encode_base64_m     (const CapeStream source);

__CAPE_LIBEX  CapeString     qcrypt__encode_base64_o     (const char* bufdat, number_t buflen);

__CAPE_LIBEX  CapeStream     qcrypt__decode_base64_s     (const CapeString source);

__CAPE_LIBEX  CapeStream     qcrypt__decode_base64_o     (const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------
// hash methods

__CAPE_LIBEX  CapeString     qcrypt__hash_sha256__hex_m  (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeString     qcrypt__hash_sha256__hex_o  (const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_sha256__bin_m  (const CapeStream source, CapeErr err);

__CAPE_LIBEX  CapeStream     qcrypt__hash_sha256__bin_o  (const char* bufdat, number_t buflen, CapeErr err);

//-----------------------------------------------------------------------------

#endif
