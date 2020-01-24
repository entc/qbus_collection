#ifndef __CAPE_FMT__JSON__H
#define __CAPE_FMT__JSON__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "stc/cape_udc.h"
#include "sys/cape_err.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc           cape_json_from_s           (const CapeString);

__CAPE_LIBEX   CapeUdc           cape_json_from_buf         (const char* buffer, number_t size);

__CAPE_LIBEX   CapeString        cape_json_to_s             (const CapeUdc source);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc           cape_json_from_file        (const CapeString file, CapeErr err);

__CAPE_LIBEX   int               cape_json_to_file          (const CapeString file, const CapeUdc source, CapeErr err);

//-----------------------------------------------------------------------------

#endif

