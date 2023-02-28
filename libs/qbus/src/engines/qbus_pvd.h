#ifndef __QBUS_PVD__H
#define __QBUS_PVD__H 1

// cape includes
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

#define QBUS_PVD_MODE_CONNECT  1
#define QBUS_PVD_MODE_LISTEN   2

typedef struct QBusPvdCtx_s* QBusPvdCtx;

//-----------------------------------------------------------------------------

typedef QBusPvdCtx     (__STDCALL *fct_qbus_pvd_ctx_new)      (CapeUdc config, CapeAioContext aio_context, CapeErr);

typedef void           (__STDCALL *fct_qbus_pvd_ctx_del)      (QBusPvdCtx*);

typedef void           (__STDCALL *fct_qbus_pvd_ctx_cb)       (QBusPvdCtx);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_pvd_ctx_new ctx_new;
  
  fct_qbus_pvd_ctx_del ctx_del;
  
  fct_qbus_pvd_ctx_cb ctx_cb;
  
} QBusPvd2;

//-----------------------------------------------------------------------------

#endif
