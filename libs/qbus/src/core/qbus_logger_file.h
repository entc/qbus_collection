#ifndef __QBUS__LOGGER_FILE__H
#define __QBUS__LOGGER_FILE__H 1

#include "qbus_types.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

__CAPE_LIBEX  QbusLogCtx   __STDCALL qbus_logger_file__dst_new (CapeAioContext aio, CapeUdc config, CapeErr err);

__CAPE_LIBEX  void         __STDCALL qbus_logger_file__dst_del (QbusLogCtx* p_self);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  void         __STDCALL qbus_logger_file__dst_msg (QbusLogCtx ctx, const CapeString remote, const CapeString message);

//-----------------------------------------------------------------------------

#endif
