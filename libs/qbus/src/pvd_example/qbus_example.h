#ifndef __QBUS_ENGINE_EXAMPLE__H
#define __QBUS_ENGINE_EXAMPLE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

#include "qbus_types.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX  QbusPvdCtx   __STDCALL qbus_pvd_ctx_new       (CapeAioContext, CapeUdc options, CapeErr);

__CAPE_LIBEX  int          __STDCALL qbus_pvd_ctx_del       (QbusPvdCtx*);

__CAPE_LIBEX  QbusPvdConn  __STDCALL qbus_pvd_ctx_add       (QbusPvdCtx, CapeUdc options, void* user_ptr, fct_qbus_pvd_factory__on_new, fct_qbus_pvd_factory__on_del, fct_qbus_pvd_cb__on_connection);

__CAPE_LIBEX  void         __STDCALL qbus_pvd_ctx_rm        (QbusPvdCtx, QbusPvdConn);

__CAPE_LIBEX  int          __STDCALL qbus_pvd_listen        (QbusPvdConn, CapeErr);

__CAPE_LIBEX  int          __STDCALL qbus_pvd_reconnect     (QbusPvdConn, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  void         __STDCALL qbus_pvd_send          (QbusPvdPhyConnection, const char* bufdat, number_t buflen, void* userdata);

__CAPE_LIBEX  void         __STDCALL qbus_pvd_mark          (QbusPvdPhyConnection);

__CAPE_LIBEX  void         __STDCALL qbus_pvd_cb_raw_set    (QbusPvdPhyConnection, fct_cape_aio_socket_onSent, fct_cape_aio_socket_onRecv);

//-----------------------------------------------------------------------------

#endif
