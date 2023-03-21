#ifndef __QBUS__H
#define __QBUS__H 1

#include "core/qbus_message.h"
#include "core/qbus_obsvbl.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBus               qbus_new               (const char* module);

__CAPE_LIBEX   void               qbus_del               (QBus*);

__CAPE_LIBEX   int                qbus_wait              (QBus, CapeUdc pvds, number_t workers, CapeErr);

//-----------------------------------------------------------------------------
                                                         /* this will initialize qbus (for testing) -> same as wait without waiting */
__CAPE_LIBEX   int                qbus_init              (QBus, CapeUdc pvds, number_t workers, CapeErr);

__CAPE_LIBEX   int                qbus_register          (QBus, const char* method, void* ptr, fct_qbus_onMessage, fct_qbus_onRemoved, CapeErr);

__CAPE_LIBEX   int                qbus_send              (QBus, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage, CapeErr);

__CAPE_LIBEX   int                qbus_continue          (QBus, const char* module, const char* method, QBusM qin, void** p_ptr, fct_qbus_onMessage, CapeErr);

__CAPE_LIBEX   int                qbus_response          (QBus, const char* module, QBusM msg, CapeErr);

__CAPE_LIBEX   const CapeString   qbus_name              (QBus);

__CAPE_LIBEX   CapeUdc            qbus_modules           (QBus);

__CAPE_LIBEX   CapeAioContext     qbus_aio               (QBus);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString   qbus_config_s          (QBus, const char* name, const CapeString default_val);

__CAPE_LIBEX   number_t           qbus_config_n          (QBus, const char* name, number_t default_val);

__CAPE_LIBEX   double             qbus_config_f          (QBus, const char* name, double default_val);

__CAPE_LIBEX   int                qbus_config_b          (QBus, const char* name, int default_val);

__CAPE_LIBEX   CapeUdc            qbus_config_node       (QBus, const char* name);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  void                qbus_log_msg           (QBus, const CapeString remote, const CapeString message);

__CAPE_LIBEX  void                qbus_log_fmt           (QBus, const CapeString remote, const char* format, ...);

//-----------------------------------------------------------------------------

typedef int      (__STDCALL     *fct_qbus_on_init) (QBus, void* ptr, void** p_ptr, CapeErr);
typedef int      (__STDCALL     *fct_qbus_on_done) (QBus, void* ptr, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_instance          (const char* name, void* ptr, fct_qbus_on_init, fct_qbus_on_done, int argc, char *argv[]);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusSubscriber     qbus_subscribe         (QBus, const CapeString module, const CapeString name, void* user_ptr, fct_qbus_on_emit);

__CAPE_LIBEX   void               qbus_emit              (QBus, const CapeString value_name, CapeUdc* p_value);

//-----------------------------------------------------------------------------

typedef void     (__STDCALL     *fct_qbus_on_route_change) (QBus, void* ptr, const CapeUdc modules);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void*              qbus_add_on_change     (QBus, void* ptr, fct_qbus_on_route_change);

__CAPE_LIBEX   void               qbus_rm_on_change      (QBus, void* obj);

//-----------------------------------------------------------------------------

typedef void     (__STDCALL     *fct_qbus_on_methods)    (QBus, void* ptr, const CapeUdc modules, CapeErr err);

__CAPE_LIBEX   void               qbus_methods           (QBus, const char* module, void* ptr, fct_qbus_on_methods);

//=============================================================================

#endif
