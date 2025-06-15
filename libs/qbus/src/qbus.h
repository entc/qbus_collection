#ifndef __QBUS__H
#define __QBUS__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

// qbus includes
#include "qbus_message.h"

//=============================================================================

struct QBus_s; typedef struct QBus_s* QBus; // use a simple version

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBus               qbus_new               (const char* module);

__CAPE_LIBEX   void               qbus_del               (QBus*);

//-----------------------------------------------------------------------------

typedef int      (__STDCALL     *fct_qbus_on_init) (QBus, void* ptr, void** p_ptr, CapeErr);
typedef int      (__STDCALL     *fct_qbus_on_done) (QBus, void* ptr, CapeErr);
typedef int      (__STDCALL     *fct_qbus_on_msg)  (QBus, void* ptr, QBusM qin, QBusM qout, CapeErr);
typedef void     (__STDCALL     *fct_qbus_on_rm)   (void* ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                qbus_register          (QBus, const CapeString method, void* user_ptr, fct_qbus_on_msg, fct_qbus_on_rm, CapeErr);

__CAPE_LIBEX   int                qbus_send              (QBus, const CapeString module, const CapeString method, QBusM msg, void* user_ptr, fct_qbus_on_msg, CapeErr);

__CAPE_LIBEX   int                qbus_continue          (QBus, const CapeString module, const CapeString method, QBusM qin, void** p_user_ptr, fct_qbus_on_msg, CapeErr);

__CAPE_LIBEX   CapeAioContext     qbus_aio               (QBus);

__CAPE_LIBEX   CapeUdc            qbus_modules           (QBus);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  void                qbus_log_msg           (QBus, const CapeString remote, const CapeString message);

__CAPE_LIBEX  void                qbus_log_fmt           (QBus, const CapeString remote, const char* format, ...);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString   qbus_config_s          (QBus, const char* name, const CapeString default_val);

__CAPE_LIBEX   number_t           qbus_config_n          (QBus, const char* name, number_t default_val);

__CAPE_LIBEX   double             qbus_config_f          (QBus, const char* name, double default_val);

__CAPE_LIBEX   int                qbus_config_b          (QBus, const char* name, int default_val);

__CAPE_LIBEX   CapeUdc            qbus_config_node       (QBus, const char* name);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_instance          (const char* name, void* ptr, fct_qbus_on_init, fct_qbus_on_done, int argc, char *argv[]);

//-----------------------------------------------------------------------------

#endif
