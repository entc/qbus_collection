#ifndef __AUTH__RINFO__H
#define __AUTH__RINFO__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

// module includes
#include "auth_tokens.h"

//-----------------------------------------------------------------------------

struct AuthRInfo_s; typedef struct AuthRInfo_s* AuthRInfo;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthRInfo  auth_rinfo_new        (AdblSession adbl_session, number_t wpid, number_t gpid);

__CAPE_LIBEX   void       auth_rino_del         (AuthRInfo*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int        auth_rinfo_get        (AuthRInfo*, CapeUdc* p_rinfo, CapeUdc* p_cdata, CapeErr err);

__CAPE_LIBEX   int        auth_rinfo_get_gpid   (AuthRInfo*, number_t gpid, CapeUdc* p_rinfo, CapeUdc* p_cdata, CapeErr err);

//-----------------------------------------------------------------------------

#endif
