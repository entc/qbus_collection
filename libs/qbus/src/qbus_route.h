#ifndef __QBUS_ROUTE__H
#define __QBUS_ROUTE__H 1

// qbus includes
#include "qbus_types.h"

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusRoute_s; typedef struct QBusRoute_s* QBusRoute;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusRoute          qbus_route_new               ();

__CAPE_LIBEX   void               qbus_route_del               (QBusRoute*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void*              qbus_route_get               (QBusRoute, const char* module);

__CAPE_LIBEX   void               qbus_route_add               (QBusRoute, const char* module, void*);

//-----------------------------------------------------------------------------

#endif 

