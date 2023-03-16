#ifndef __QBUS_ROUTE__H
#define __QBUS_ROUTE__H 1

#include "qbus_frame.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

// predefine the engines here
struct QBusEngines_s; typedef struct QBusEngines_s* QBusEngines;
struct QBusEnginesPvd_s; typedef struct QBusEnginesPvd_s* QBusEnginesPvd;

// predefine observable classes
struct QBusObsvbl_s; typedef struct QBusObsvbl_s* QBusObsvbl;
struct QBusSubscriber_s; typedef struct QBusSubscriber_s* QBusSubscriber;

//-----------------------------------------------------------------------------

struct QBusPvdConnection_s
{
  void* connection_ptr;
  QBusEnginesPvd engine;
  number_t version;
  
}; typedef struct QBusPvdConnection_s* QBusPvdConnection;

//-----------------------------------------------------------------------------

struct QBusRouteNameItem_s; typedef struct QBusRouteNameItem_s* QBusRouteNameItem;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   const CapeString   qbus_route_name_uuid_get     (QBusRouteNameItem);

__CAPE_LOCAL   void               qbus_route_name_dump2        (QBusRouteNameItem, const CapeString module_name, int is_local, const CapeString data);

//-----------------------------------------------------------------------------

struct QBusRoute_s; typedef struct QBusRoute_s* QBusRoute;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusRoute          qbus_route_new               (const CapeString name, QBusEngines);

__CAPE_LOCAL   void               qbus_route_del               (QBusRoute*);

__CAPE_LOCAL   void               qbus_route_set               (QBusRoute, QBusObsvbl);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   const CapeString   qbus_route_uuid_get          (QBusRoute);

__CAPE_LOCAL   const CapeString   qbus_route_name_get          (QBusRoute);

__CAPE_LOCAL   CapeUdc            qbus_route_node_get          (QBusRoute, int as_node, QBusPvdConnection conn_not_in_list);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_route_add_nodes         (QBusRoute, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection, CapeUdc*);

__CAPE_LOCAL   void               qbus_route_rm                (QBusRoute, QBusPvdConnection);

__CAPE_LOCAL   void               qbus_route_send_update       (QBusRoute, QBusPvdConnection not_in_list, QBusPvdConnection single);

__CAPE_LOCAL   void               qbus_route_dump              (QBusRoute);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_route_get__conn         (QBusRoute, QBusPvdConnection conn, CapeList user_ptrs__version1, CapeList user_ptrs__version3);

__CAPE_LOCAL   CapeMap            qbus_route_get__routings     (QBusRoute, CapeMap);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_route_frame_nodes_add   (QBusRoute, QBusFrame frame, int as_node, QBusPvdConnection conn_not_in_list);

//-----------------------------------------------------------------------------

#endif
