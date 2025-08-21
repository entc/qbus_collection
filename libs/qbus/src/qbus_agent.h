#ifndef __QBUS_AGENT__H
#define __QBUS_AGENT__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "aio/cape_aio_ctx.h"

#include "qbus_router.h"

//-----------------------------------------------------------------------------

struct QBusAgent_s; typedef struct QBusAgent_s* QBusAgent;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusAgent           qbus_agent_new          (QBusRouter);

__CAPE_LIBEX   void                qbus_agent_del          (QBusAgent*);

__CAPE_LIBEX   int                 qbus_agent_init         (QBusAgent, CapeAioContext, number_t port, CapeErr err);

//-----------------------------------------------------------------------------

#endif
