#ifndef __QBUS__LOGGER__H
#define __QBUS__LOGGER__H 1

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

typedef struct QBusLogger_s* QBusLogger;

//-----------------------------------------------------------------------------

__CAPE_LIBEX  QBusLogger      qbus_logger_new        ();

__CAPE_LIBEX  void            qbus_logger_del        (QBusLogger* p_self);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  int             qbus_logger_init       (QBusLogger, CapeAioContext, CapeUdc, CapeErr);

__CAPE_LIBEX  void            qbus_logger_msg        (QBusLogger, const CapeString remote, const CapeString message);

//-----------------------------------------------------------------------------

#endif
