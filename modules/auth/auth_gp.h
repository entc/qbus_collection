#ifndef __AUTH__GP__H
#define __AUTH__GP__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct AuthGP_s; typedef struct AuthGP_s* AuthGP;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthGP   auth_gp_new    (AdblSession adbl_session);

__CAPE_LIBEX   void     auth_gp_del    (AuthGP*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int      auth_gp_get     (AuthGP*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif
