#ifndef __QBUS_ENGINE_TCP__H
#define __QBUS_ENGINE_TCP__H 1

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

#include "qbus_pvd.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusPvdCtx      __STDCALL pvd2_ctx_new                (CapeAioContext aio_context, fct_qbus_pvd__fcts_new, fct_qbus_pvd__fcts_del, void* factory_ptr, CapeErr);

__CAPE_LIBEX   void            __STDCALL pvd2_ctx_del                (QBusPvdCtx*);

__CAPE_LIBEX   void            __STDCALL pvd2_ctx_reg                (QBusPvdCtx, CapeUdc config);

__CAPE_LIBEX   void            __STDCALL pvd2_ctx_send               (QBusPvdCtx, void* pfd_ptr, QBusFrame frame);

__CAPE_LOCAL   void                      pvd2_ctx__signal_thread     (QBusPvdCtx, char signal_msg);

__CAPE_LOCAL   void                      pvd2_ctx__register_handle   (QBusPvdCtx, QBusPvdEntity entity, void* handle, int handle_type);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusPvdEntity             pvd2_entity_new             (QBusPvdCtx, int reconnect, const CapeString host, number_t port);

__CAPE_LOCAL   void                      pvd2_entity_del             (QBusPvdEntity*);

__CAPE_LOCAL   void                      pvd2_entity_fcts_set        (QBusPvdEntity, QBusPvdFcts* p_fcts);

//-----------------------------------------------------------------------------

#endif
