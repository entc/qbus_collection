#ifndef __QBUS_ENGINES__H
#define __QBUS_ENGINES__H 1

#include "qbus_route.h"
#include "qbus_frame.h"
#include "qbus_obsvbl.h"
#include "qbus_methods.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusEngines  qbus_engines_new                 ();

__CAPE_LOCAL   void         qbus_engines_del                 (QBusEngines*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   int          qbus_engines__init_pvds          (QBusEngines, const CapeUdc pvds, CapeAioContext aio_context, QBusRoute, QBusObsvbl, QBusMethods, CapeErr err);

__CAPE_LOCAL   void         qbus_engines__send               (QBusEngines, QBusFrame frame, QBusPvdConnection conn);

__CAPE_LOCAL   void         qbus_engines__broadcast          (QBusEngines, QBusFrame frame, CapeList user_ptrs);

//-----------------------------------------------------------------------------

#endif
