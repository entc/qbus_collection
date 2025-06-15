#ifndef __QBUS_ROUTER__H
#define __QBUS_ROUTER__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

#include "qbus_pvd.h"

//-----------------------------------------------------------------------------

struct QBusRouter_s; typedef struct QBusRouter_s* QBusRouter;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusRouter           qbus_router_new       ();

__CAPE_LIBEX   void                 qbus_router_del       (QBusRouter*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                 qbus_router_add       (QBusRouter, const CapeString cid, const CapeString name);

__CAPE_LIBEX   void                 qbus_router_rm        (QBusRouter, const CapeString cid, const CapeString name);

__CAPE_LIBEX   const CapeString     qbus_router_get       (QBusRouter, const CapeString name);

__CAPE_LIBEX   CapeUdc              qbus_router_list      (QBusRouter);

//-----------------------------------------------------------------------------

#endif
