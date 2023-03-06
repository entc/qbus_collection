#ifndef __QBUS_ROUTE__H
#define __QBUS_ROUTE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

struct QBusRoute_s; typedef struct QBusRoute_s* QBusRoute;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusRoute          qbus_route_new               (const CapeString name);

__CAPE_LOCAL   void               qbus_route_del               (QBusRoute*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   const CapeString   qbus_route_uuid_get          (QBusRoute);

__CAPE_LOCAL   const CapeString   qbus_route_name_get          (QBusRoute);

__CAPE_LOCAL   CapeUdc            qbus_route_node_get          (QBusRoute);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_route_add_nodes         (QBusRoute, const CapeString module_name, const CapeString module_uuid, void* user_ptr, CapeUdc*);

__CAPE_LOCAL   void               qbus_route_dump              (QBusRoute);

//-----------------------------------------------------------------------------

#endif
