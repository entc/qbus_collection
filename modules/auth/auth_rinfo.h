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

__CAPE_LIBEX   AuthRInfo  auth_rinfo_new  (AdblSession adbl_session, number_t userid, number_t wpid);

__CAPE_LIBEX   void       auth_rino_del   (AuthRInfo*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int        auth_rinfo_get  (AuthRInfo*, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif
