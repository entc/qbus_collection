#ifndef __QBUS_METHODS__H
#define __QBUS_METHODS__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

#include "qbus_message.h"
#include "qbus.h"

//-----------------------------------------------------------------------------

struct QBusMethods_s; typedef struct QBusMethods_s* QBusMethods;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusMethods          qbus_methods_new          ();

__CAPE_LIBEX   void                 qbus_methods_del          (QBusMethods*);

__CAPE_LIBEX   int                  qbus_methods_add          (QBusMethods, const CapeString method, void* user_ptr, fct_qbus_on_msg, fct_qbus_on_rm, CapeErr err);

__CAPE_LIBEX   int                  qbus_methods_run          (QBusMethods, const CapeString method, CapeErr err);

//-----------------------------------------------------------------------------

#endif
