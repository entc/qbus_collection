#ifndef __QBUS_ENGINES_PVD__H
#define __QBUS_ENGINES_PVD__H 1

#include "qbus_route.h"

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

struct QBusEnginesPvd_s; typedef struct QBusEnginesPvd_s* QBusEnginesPvd;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusEnginesPvd     qbus_engines_pvd_new          (QBusRoute);

__CAPE_LOCAL   void               qbus_engines_pvd_del          (QBusEnginesPvd*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   int                qbus_engines_pvd_load         (QBusEnginesPvd, const CapeString path, const CapeString name, CapeAioContext aio_context, CapeErr err);

__CAPE_LOCAL   int                qbus_engines_pvd__entity_new  (QBusEnginesPvd, const CapeUdc config, CapeErr err);

//-----------------------------------------------------------------------------

#endif
