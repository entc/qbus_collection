#ifndef __QBUS_ENGINES__H
#define __QBUS_ENGINES__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

#include "qbus_pvd.h"

//-----------------------------------------------------------------------------

struct QBusEngine_s; typedef struct QBusEngine_s* QBusEngine;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusEngine           qbus_engine_new       (const CapeString path, const CapeString engine_name, CapeErr err);

__CAPE_LIBEX   void                 qbus_engine_del       (QBusEngine*);

//-----------------------------------------------------------------------------
// context methods

__CAPE_LIBEX   QbusPvdCtx           qbus_engine_ctx_new   (QBusEngine, CapeAioContext aio, const CapeString name, CapeErr err);

__CAPE_LIBEX   void                 qbus_engine_ctx_add   (QBusEngine, QbusPvdCtx, QbusPvdConnection*, CapeUdc options, void* user_ptr, fct_qbus_pvd__on_con, fct_qbus_pvd__on_snd);

//-----------------------------------------------------------------------------
// connection methods

__STDCALL      const CapeString     qbus_engine_con_cid   (QBusEngine, QbusPvdConnection);

__STDCALL      void                 qbus_engine_con_snd   (QBusEngine, QbusPvdConnection, const CapeString cid, QBusFrame frame);

__STDCALL      void                 qbus_engine_con_subscribe   (QBusEngine, QbusPvdConnection, const CapeString topic);

__STDCALL      void                 qbus_engine_con_unsubscribe   (QBusEngine, QbusPvdConnection, const CapeString topic);

__STDCALL      void                 qbus_engine_con_next   (QBusEngine, QbusPvdConnection, const CapeString topic, QBusFrame frame);

//-----------------------------------------------------------------------------

struct QBusEngines_s; typedef struct QBusEngines_s* QBusEngines;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusEngines          qbus_engines_new      (const CapeString engines_path);

__CAPE_LIBEX   void                 qbus_engines_del      (QBusEngines*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusEngine           qbus_engines_add      (QBusEngines, const CapeString engine_name, CapeErr err);

//-----------------------------------------------------------------------------

#endif
