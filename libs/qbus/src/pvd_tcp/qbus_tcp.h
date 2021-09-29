#ifndef __QBUS_ENGINE_TCP__H
#define __QBUS_ENGINE_TCP__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

#include "qbus_types.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX  QbusPvdCtx     __STDCALL qbus_pvd_ctx_new       (CapeAioContext, CapeUdc options, CapeErr);

__CAPE_LIBEX  int            __STDCALL qbus_pvd_ctx_del       (QbusPvdCtx*);

__CAPE_LIBEX  QbusPvdEntity  __STDCALL qbus_pvd_ctx_add       (QbusPvdCtx, CapeUdc options, void* user_ptr, fct_qbus_pvd_factory__on_new, fct_qbus_pvd_factory__on_del, fct_qbus_pvd_cb__on_connection);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_ctx_rm        (QbusPvdCtx, QbusPvdEntity);

__CAPE_LIBEX  int            __STDCALL qbus_pvd_listen        (QbusPvdEntity, CapeErr);

__CAPE_LIBEX  int            __STDCALL qbus_pvd_reconnect     (QbusPvdEntity, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  void           __STDCALL qbus_pvd_send          (QbusPvdConnection, const char* bufdat, number_t buflen, void* userdata);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_mark          (QbusPvdConnection);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_cb_raw_set    (QbusPvdConnection, fct_cape_aio_socket_onSent, fct_cape_aio_socket_onRecv);

//-----------------------------------------------------------------------------

#endif
