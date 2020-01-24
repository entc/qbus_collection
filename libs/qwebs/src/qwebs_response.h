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

__CAPE_LIBEX   void      qwebs_response_err       (CapeStream s, QWebs webs, CapeUdc content, const CapeString mime, CapeErr);

//-----------------------------------------------------------------------------

#endif

