#ifndef __QBUS_ENGINES_PVD__H
#define __QBUS_ENGINES_PVD__H 1

#include "qbus_route.h"
#include "qbus_frame.h"
#include "qbus_obsvbl.h"
#include "qbus_methods.h"

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusEnginesPvd     qbus_engines_pvd_new          (QBusRoute, QBusObsvbl, QBusMethods);

__CAPE_LOCAL   void               qbus_engines_pvd_del          (QBusEnginesPvd*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   int                qbus_engines_pvd_load         (QBusEnginesPvd, const CapeString path, const CapeString name, CapeAioContext aio_context, CapeErr err);

__CAPE_LOCAL   void               qbus_engines_pvd_send         (QBusEnginesPvd, QBusFrame frame, void* connection_ptr);

__CAPE_LOCAL   int                qbus_engines_pvd__entity_new  (QBusEnginesPvd, const CapeUdc config, CapeErr err);

//-----------------------------------------------------------------------------

#endif
