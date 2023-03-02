#ifndef __QBUS_PVD__H
#define __QBUS_PVD__H 1

// cape includes
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

#define QBUS_PVD_MODE_CONNECT  1
#define QBUS_PVD_MODE_LISTEN   2

typedef struct QBusPvdCtx_s* QBusPvdCtx;
typedef struct QBusPvdEntity_s* QBusPvdEntity;

//-----------------------------------------------------------------------------

typedef QBusPvdCtx     (__STDCALL *fct_qbus_pvd_ctx_new)          (CapeAioContext aio_context, CapeErr);

typedef void           (__STDCALL *fct_qbus_pvd_ctx_del)          (QBusPvdCtx*);

//-----------------------------------------------------------------------------

typedef QBusPvdEntity  (__STDCALL *fct_qbus_pvd_entity_new)       (QBusPvdCtx, CapeUdc config, CapeErr);

typedef void           (__STDCALL *fct_qbus_pvd_entity_del)       (QBusPvdEntity*);

typedef void           (__STDCALL *fct_qbus_pvd_entity_cb)        (QBusPvdEntity);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_pvd_ctx_new      ctx_new;

  fct_qbus_pvd_ctx_del      ctx_del;
  
  fct_qbus_pvd_entity_new   entity_new;
  
  fct_qbus_pvd_entity_del   entity_del;
  
  fct_qbus_pvd_entity_cb    entity_cb;
  
} QBusPvd2;

//-----------------------------------------------------------------------------

#endif
