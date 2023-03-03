#ifndef __QBUS_PVD__H
#define __QBUS_PVD__H 1

// cape includes
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

#define QBUS_PVD_MODE_DISABLED  0
#define QBUS_PVD_MODE_CLIENT    1
#define QBUS_PVD_MODE_LISTEN    2
#define QBUS_PVD_MODE_REMOTE    3

typedef struct QBusPvdCtx_s* QBusPvdCtx;
typedef struct QBusPvdEntity_s* QBusPvdEntity;

//-----------------------------------------------------------------------------

typedef struct 
{
  
  
  
} QBusPvdFcts;

//-----------------------------------------------------------------------------

typedef QBusPvdCtx     (__STDCALL *fct_qbus_pvd_ctx_new)          (CapeAioContext aio_context, CapeErr);

typedef void           (__STDCALL *fct_qbus_pvd_ctx_del)          (QBusPvdCtx*);

typedef void           (__STDCALL *fct_qbus_pvd_ctx_reg)          (QBusPvdCtx, CapeUdc config, QBusPvdFcts* fcts, void* user_ptr);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_pvd_ctx_new      ctx_new;

  fct_qbus_pvd_ctx_del      ctx_del;
  
  fct_qbus_pvd_ctx_reg      ctx_reg;
  
} QBusPvd2;

//-----------------------------------------------------------------------------

#endif
