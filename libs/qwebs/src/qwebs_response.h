#ifndef __QWEBS_RESPONSE__H
#define __QWEBS_RESPONSE__H 1

#include "qwebs_structs.h"

//-----------------------------------------------------------------------------

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include <stc/cape_stream.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void      qwebs_response_file      (CapeStream s, QWebs webs, CapeUdc file_node);

__CAPE_LIBEX   void      qwebs_response_json      (CapeStream s, QWebs webs, CapeUdc content);

__CAPE_LIBEX   void      qwebs_response_image     (CapeStream s, QWebs webs, const CapeString image_as_base64);

__CAPE_LIBEX   void      qwebs_response_buf       (CapeStream s, QWebs webs, const CapeString buf, const CapeString mime_type);

__CAPE_LIBEX   void      qwebs_response_err       (CapeStream s, QWebs webs, CapeUdc content, const CapeString mime, CapeErr);

__CAPE_LIBEX   void      qwebs_response_redirect  (CapeStream s, QWebs webs, const CapeString url);

__CAPE_LIBEX   void      qwebs_response_mp_init   (CapeStream s, QWebs webs, const CapeString boundary, const CapeString mime);

__CAPE_LIBEX   void      qwebs_response_mp_part   (CapeStream s, QWebs webs, const CapeString boundary, const CapeString mime, const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------

#endif

