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

typedef void           (__STDCALL *fct_qbus_pvd__on_connect)      (void* user_ptr);

typedef void           (__STDCALL *fct_qbus_pvd__on_disconnect)   (void* user_ptr);

//-----------------------------------------------------------------------------

typedef struct 
{
  fct_qbus_pvd__on_connect         on_connect;
  fct_qbus_pvd__on_disconnect      on_disconnect;
  
  void* user_ptr;
  
} QBusPvdFcts;

typedef QBusPvdFcts*   (__STDCALL *fct_qbus_pvd__fcts_new)        (void* factory_ptr);

typedef void           (__STDCALL *fct_qbus_pvd__fcts_del)        (QBusPvdFcts**);

//-----------------------------------------------------------------------------

typedef QBusPvdCtx     (__STDCALL *fct_qbus_pvd_ctx_new)          (CapeAioContext aio_context, fct_qbus_pvd__fcts_new, fct_qbus_pvd__fcts_del, void* factory_ptr, CapeErr);

typedef void           (__STDCALL *fct_qbus_pvd_ctx_del)          (QBusPvdCtx*);

typedef void           (__STDCALL *fct_qbus_pvd_ctx_reg)          (QBusPvdCtx, CapeUdc config);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_pvd_ctx_new      ctx_new;

  fct_qbus_pvd_ctx_del      ctx_del;
  
  fct_qbus_pvd_ctx_reg      ctx_reg;
  
} QBusPvd2;

//-----------------------------------------------------------------------------

#endif
