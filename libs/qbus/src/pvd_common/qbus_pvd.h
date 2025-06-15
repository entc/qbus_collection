#ifndef __QBUS_PVD__H
#define __QBUS_PVD__H 1

// cape includes
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>
#include <aio/cape_aio_sock.h>

#include "qbus_frame.h"

//-----------------------------------------------------------------------------
// object typedefs and definitions

typedef struct QbusPvdCtx_s* QbusPvdCtx;
typedef struct QbusPvdEntity_s* QbusPvdEntity;
typedef struct QbusPvdConnection_s* QbusPvdConnection;

//-----------------------------------------------------------------------------

                       /* callback after a new connection was established */
typedef void           (__STDCALL *fct_qbus_pvd__on_con) (void* user_ptr, const CapeString cid, const CapeString name, number_t type);

typedef void           (__STDCALL *fct_qbus_pvd__on_snd) (void* user_ptr, QBusFrame frame);

//-----------------------------------------------------------------------------
// library functions

typedef int            (__STDCALL *fct_qbus_pvd_init)         (CapeErr);

typedef void           (__STDCALL *fct_qbus_pvd_done)         (void);

//-----------------------------------------------------------------------------
// context functions

typedef QbusPvdCtx         (__STDCALL *fct_qbus_pvd_ctx_new)      (CapeAioContext, CapeUdc, CapeErr);

typedef void               (__STDCALL *fct_qbus_pvd_ctx_del)      (QbusPvdCtx*);

typedef void               (__STDCALL *fct_qbus_pvd_ctx_add)      (QbusPvdCtx, QbusPvdConnection*, CapeUdc options, void* user_ptr, fct_qbus_pvd__on_con, fct_qbus_pvd__on_snd);

//-----------------------------------------------------------------------------
// connection functions

typedef void               (__STDCALL *fct_qbus_pvd_con_snd)      (QbusPvdConnection, const CapeString cid, QBusFrame frame);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_pvd_init            pvd_init;
  fct_qbus_pvd_done            pvd_done;

  fct_qbus_pvd_ctx_new         pvd_ctx_new;
  fct_qbus_pvd_ctx_del         pvd_ctx_del;
  fct_qbus_pvd_ctx_add         pvd_ctx_add;

  fct_qbus_pvd_con_snd         pvd_con_snd;
  
} QbusPvd;

#endif
