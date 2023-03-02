#ifndef __QBUS_ENGINE_TCP__H
#define __QBUS_ENGINE_TCP__H 1

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

#include "qbus_pvd.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusPvdCtx      __STDCALL pvd2_ctx_new         (CapeAioContext aio_context, CapeErr);

__CAPE_LIBEX   void            __STDCALL pvd2_ctx_del         (QBusPvdCtx*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusPvdEntity   __STDCALL pvd2_entity_new      (QBusPvdCtx, CapeUdc config, CapeErr);

__CAPE_LIBEX   void            __STDCALL pvd2_entity_del      (QBusPvdEntity*);

__CAPE_LIBEX   void            __STDCALL pvd2_entity_cb       (QBusPvdEntity);

//-----------------------------------------------------------------------------

#endif
