#ifndef __QBUS_ENGINES__H
#define __QBUS_ENGINES__H 1

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

struct QBusEngines_s; typedef struct QBusEngines_s* QBusEngines;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusEngines  qbus_engines_new                 ();

__CAPE_LOCAL   void         qbus_engines_del                 (QBusEngines*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   int          qbus_engines__init_pvds          (QBusEngines, const CapeUdc pvds, CapeAioContext aio_context, CapeErr err);

//-----------------------------------------------------------------------------

#endif
