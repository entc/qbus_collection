#ifndef __QBUS_TYPES__H
#define __QBUS_TYPES__H 1

// cape includes
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>
#include <aio/cape_aio_sock.h>

//-----------------------------------------------------------------------------

typedef struct QbusPvdCtx_s* QbusPvdCtx;
typedef struct QbusPvdConn_s* QbusPvdConn;
typedef struct QbusPvdPhyConnection_s* QbusPvdPhyConnection;

//-----------------------------------------------------------------------------

                     /* factory callback to create an user defined object to handle the upper layer logic */
typedef void*        (__STDCALL *fct_qbus_pvd_factory__on_new)   (void* user_ptr, QbusPvdPhyConnection phy_connection);

                     /* factory callback to release the upper layer object */
typedef void         (__STDCALL *fct_qbus_pvd_factory__on_del)   (void** object_ptr);

                     /* callback after a new connection was established */
typedef void*        (__STDCALL *fct_qbus_pvd_cb__on_connection) (void* object_ptr);

//-----------------------------------------------------------------------------

typedef QbusPvdCtx   (__STDCALL *fct_qbus_pvd_ctx_new)      (CapeAioContext, CapeUdc, CapeErr);

typedef int          (__STDCALL *fct_qbus_pvd_ctx_del)      (QbusPvdCtx*);

typedef QbusPvdConn  (__STDCALL *fct_qbus_pvd_ctx_add)      (QbusPvdCtx, CapeUdc options, void* user_ptr, fct_qbus_pvd_factory__on_new, fct_qbus_pvd_factory__on_del, fct_qbus_pvd_cb__on_connection);

typedef void         (__STDCALL *fct_qbus_pvd_ctx_rm)       (QbusPvdCtx, QbusPvdConn);

typedef int          (__STDCALL *fct_qbus_pvd_listen)       (QbusPvdConn, CapeErr);

typedef int          (__STDCALL *fct_qbus_pvd_reconnect)    (QbusPvdConn, CapeErr);

//-----------------------------------------------------------------------------

typedef void         (__STDCALL *fct_qbus_pvd_send)         (QbusPvdPhyConnection, const char* bufdat, number_t buflen, void* userdata);

typedef void         (__STDCALL *fct_qbus_pvd_mark)         (QbusPvdPhyConnection);

typedef void         (__STDCALL *fct_qbus_pvd_cb_raw_set)   (QbusPvdPhyConnection, fct_cape_aio_socket_onSent, fct_cape_aio_socket_onRecv);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_pvd_ctx_new         pvd_ctx_new;
  fct_qbus_pvd_ctx_del         pvd_ctx_del;
  fct_qbus_pvd_ctx_add         pvd_ctx_add;
  fct_qbus_pvd_ctx_rm          pvd_ctx_rm;
  fct_qbus_pvd_listen          pvd_listen;
  fct_qbus_pvd_reconnect       pvd_reconnect;

  fct_qbus_pvd_send            pvd_send;
  fct_qbus_pvd_mark            pvd_mark;
  fct_qbus_pvd_cb_raw_set      pvd_cb_raw_set;
  
} QbusPvd;

//-----------------------------------------------------------------------------

#endif
