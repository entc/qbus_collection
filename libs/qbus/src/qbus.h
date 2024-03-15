#ifndef __QBUS__H
#define __QBUS__H 1

#include "qbus_method.h"
#include "qbus_types.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

// TopLevel1 qbus includes
#include "tl1/qbus_tl1.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBus               qbus_new               (const char* module, QBusManifold);

__CAPE_LIBEX   void               qbus_del               (QBus*);

__CAPE_LIBEX   int                qbus_wait              (QBus, CapeUdc bind, CapeUdc remotes, number_t workers, CapeErr);

__CAPE_LIBEX   int                qbus_next              (QBus, CapeErr);

//-----------------------------------------------------------------------------

#define QBUS_MTYPE_NONE         0
#define QBUS_MTYPE_JSON         1
#define QBUS_MTYPE_FILE         2

//-----------------------------------------------------------------------------

                                                         /* this will initialize qbus (for testing) -> same as wait without waiting */
__CAPE_LIBEX   int                qbus_init              (QBus, number_t workers, CapeErr);

__CAPE_LIBEX   int                qbus_register          (QBus, const char* method, void* ptr, fct_qbus_onMessage, fct_qbus_onRemoved, CapeErr);

__CAPE_LIBEX   int                qbus_send              (QBus, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage, CapeErr);

__CAPE_LIBEX   int                qbus_test_s            (QBus, const char* module, const char* method, CapeErr);   // Called from java with JNI

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

struct QBusConnection_s; typedef struct QBusConnection_s* QBusConnection;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusConnection const  qbus_find_conn      (QBus, const char* module);

__CAPE_LIBEX   void               qbus_conn_request      (QBus, QBusConnection const, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusM              qbus_message_new       (const CapeString key, const CapeString sender);

__CAPE_LIBEX   void               qbus_message_del       (QBusM*);

__CAPE_LIBEX   void               qbus_message_clr       (QBusM, u_t cdata_udc_type);

__CAPE_LIBEX   QBusM              qbus_message_data_mv   (QBusM);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                qbus_message_role_has  (QBusM, const CapeString role_name);

__CAPE_LIBEX   int                qbus_message_role_or2  (QBusM, const CapeString role01, const CapeString role02);

//-----------------------------------------------------------------------------

typedef int      (__STDCALL     *fct_qbus_on_init) (QBus, void* ptr, void** p_ptr, CapeErr);
typedef int      (__STDCALL     *fct_qbus_on_done) (QBus, void* ptr, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_instance          (const char* name, void* ptr, fct_qbus_on_init, fct_qbus_on_done, int argc, char *argv[]);

//-----------------------------------------------------------------------------

struct QBusSubscriber_s; typedef struct QBusSubscriber_s* QBusSubscriber;
struct QBusEmitter_s; typedef struct QBusEmitter_s* QBusEmitter;

typedef int      (__STDCALL     *fct_qbus_on_emit) (QBusSubscriber, void* user_ptr, int type, CapeUdc data, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusSubscriber     qbus_subscribe         (QBus, int type, const CapeString module, const CapeString name, CapeErr);

__CAPE_LIBEX   QBusEmitter        qbus_emitter_add       (QBus, const CapeString name, CapeErr);

__CAPE_LIBEX   int                qbus_emitter_rm        (QBus, QBusEmitter, CapeErr);

__CAPE_LIBEX   void               qbus_emitter_next      (QBus, QBusEmitter, CapeUdc data);

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
