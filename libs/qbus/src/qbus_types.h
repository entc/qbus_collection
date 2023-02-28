#ifndef __QBUS_TYPES__H
#define __QBUS_TYPES__H 1

// cape includes
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>
#include <aio/cape_aio_sock.h>

//-----------------------------------------------------------------------------

typedef struct QbusLogCtx_s* QbusLogCtx;

//-----------------------------------------------------------------------------

typedef QbusLogCtx     (__STDCALL *fct_qbus_log_dst_new)      (CapeAioContext, CapeUdc, CapeErr);

typedef void           (__STDCALL *fct_qbus_log_dst_del)      (QbusLogCtx*);

typedef void           (__STDCALL *fct_qbus_log_dst_msg)      (QbusLogCtx, const CapeString remote, const CapeString message);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_log_dst_new         log_dst_new;
  fct_qbus_log_dst_del         log_dst_del;
  fct_qbus_log_dst_msg         log_dst_msg;
  
} QbusLogDst;


#endif
