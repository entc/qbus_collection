#ifndef __QBUS_PVD__H
#define __QBUS_PVD__H 1

#include "qbus_message.h"
#include "qbus_frame.h"
#include "qbus_route.h"

// cape includes
#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

#define QBUS_PVD_MODE_DISABLED  0
#define QBUS_PVD_MODE_CLIENT    1
#define QBUS_PVD_MODE_LISTEN    2
#define QBUS_PVD_MODE_REMOTE    3
#define QBUS_PVD_MODE_CONNECT   4

//-----------------------------------------------------------------------------

typedef struct QBusPvdCtx_s* QBusPvdCtx;
typedef struct QBusPvdEntity_s* QBusPvdEntity;

//-----------------------------------------------------------------------------

typedef void           (__STDCALL *fct_qbus_pvd__on_frame)        (void* factory_ptr, QBusPvdConnection conn, QBusFrame* p_frame);

//-----------------------------------------------------------------------------

struct QBusPvdFcts_s
{
  fct_qbus_pvd__on_frame           on_frame;
  
  QBusPvdConnection conn;
  
}; typedef struct QBusPvdFcts_s* QBusPvdFcts;

//-----------------------------------------------------------------------------

                          /* factory method to create a new functions structure */
typedef QBusPvdFcts       (__STDCALL *fct_qbus_pvd__fcts_new)        (void* factory_ptr, void* connection_ptr);

                          /* factory method to release the function structure */
typedef void              (__STDCALL *fct_qbus_pvd__fcts_del)        (void* factory_ptr, int reconnect, QBusPvdFcts*);

//-----------------------------------------------------------------------------

typedef QBusPvdCtx        (__STDCALL *fct_qbus_pvd_ctx_new)          (CapeAioContext aio_context, fct_qbus_pvd__fcts_new, fct_qbus_pvd__fcts_del, void* factory_ptr, CapeErr);

typedef void              (__STDCALL *fct_qbus_pvd_ctx_del)          (QBusPvdCtx*);

typedef void              (__STDCALL *fct_qbus_pvd_ctx_reg)          (QBusPvdCtx, CapeUdc config);

typedef void              (__STDCALL *fct_qbus_pvd_ctx_send)         (QBusPvdCtx, void* connection_ptr, QBusFrame frame);

//-----------------------------------------------------------------------------

#endif
