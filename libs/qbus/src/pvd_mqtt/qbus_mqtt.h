#ifndef __QBUS_PVD_MQTT__H
#define __QBUS_PVD_MQTT__H 1

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

#include "qbus_pvd.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX  int            __STDCALL qbus_pvd_init             (CapeErr err);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_done             ();

__CAPE_LIBEX  QbusPvdCtx     __STDCALL qbus_pvd_ctx_new          (CapeAioContext, CapeUdc options, CapeErr);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_ctx_del          (QbusPvdCtx*);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_con_snd          (QbusPvdConnection, const CapeString cid, QBusFrame frame);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_con_subscribe    (QbusPvdConnection, const CapeString topic);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_con_unsubscribe  (QbusPvdConnection, const CapeString topic);

__CAPE_LIBEX  void           __STDCALL qbus_pvd_con_next         (QbusPvdConnection, const CapeString topic, QBusFrame frame);

//-----------------------------------------------------------------------------

#endif
