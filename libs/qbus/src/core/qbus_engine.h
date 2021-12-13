#ifndef __QBUS__ENGINE__H
#define __QBUS__ENGINE__H 1

#include "qbus_route.h"
#include "qbus_types.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

typedef struct QBusEngine_s* QBusEngine;

//-----------------------------------------------------------------------------

__CAPE_LIBEX  QBusEngine      qbus_engine_new        (QBusRoute route);

__CAPE_LIBEX  void            qbus_engine_del        (QBusEngine* p_self);

__CAPE_LIBEX  int             qbus_engine_load       (QBusEngine, const CapeString path, const CapeString name, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  QbusPvdCtx      qbus_engine_ctx_add    (QBusEngine, CapeAioContext aio, const CapeString name, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  QbusPvdEntity   qbus_engine_add        (QBusEngine, QbusPvdCtx ctx, CapeUdc options, CapeErr);

__CAPE_LIBEX  int             qbus_engine_reconnect  (QBusEngine, QbusPvdEntity entity, CapeErr);

__CAPE_LIBEX  int             qbus_engine_listen     (QBusEngine, QbusPvdEntity entity, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  QBusEngine      qbus_engine_default    (QBusRoute route);

//-----------------------------------------------------------------------------

#endif
