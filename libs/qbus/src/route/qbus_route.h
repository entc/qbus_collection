#ifndef __QBUS_ROUTE__H
#define __QBUS_ROUTE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

struct QBusRoute_s; typedef struct QBusRoute_s* QBusRoute;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusRoute          qbus_route_new               ();

__CAPE_LOCAL   void               qbus_route_del               (QBusRoute*);

//-----------------------------------------------------------------------------

#endif
