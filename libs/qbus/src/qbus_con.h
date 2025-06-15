#ifndef __QBUS_CON__H
#define __QBUS_CON__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

#include "qbus_engines.h"
#include "qbus_router.h"
#include "qbus_message.h"

//-----------------------------------------------------------------------------

struct QBusCon_s; typedef struct QBusCon_s* QBusCon;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusCon              qbus_con_new          (QBusRouter router, const CapeString module_name);

__CAPE_LIBEX   void                 qbus_con_del          (QBusCon*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                  qbus_con_init         (QBusCon, QBusEngines engines, CapeAioContext, CapeErr err);

__CAPE_LIBEX   void                 qbus_con_snd          (QBusCon, const CapeString cid, QBusM);

//-----------------------------------------------------------------------------

#endif
