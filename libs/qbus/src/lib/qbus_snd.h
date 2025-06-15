#ifndef __QBUS_SND__H
#define __QBUS_SND__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

#include "qbus_engines.h"
#include "qbus_router.h"

//-----------------------------------------------------------------------------

struct QBusSnd_s; typedef struct QBusSnd_s* QBusSnd;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusSnd              qbus_snd_new          ();

__CAPE_LIBEX   void                 qbus_snd_del          (QBusSnd*);

//-----------------------------------------------------------------------------

#endif
