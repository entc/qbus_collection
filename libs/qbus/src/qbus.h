#ifndef __QBUS__H
#define __QBUS__H 1

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <stc/cape_stream.h>
#include <aio/cape_aio_ctx.h>

// qbus includes
#include "qbus_message.h"
#include "qbus_config.h"

// for backward compatibility
#define fct_qbus_onMessage fct_qbus_on_msg

//=============================================================================

struct QBus_s; typedef struct QBus_s* QBus; // use a simple version

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBus               qbus_new               (const CapeString module);

__CAPE_LIBEX   void               qbus_del               (QBus*);

__CAPE_LIBEX   int                qbus_init              (QBus, CapeUdc* p_args, CapeErr err);

__CAPE_LIBEX   int                qbus_wait              (QBus, CapeUdc* p_args, CapeErr err);

//-----------------------------------------------------------------------------

typedef int      (__STDCALL     *fct_qbus_on_init) (QBus, void* ptr, void** p_ptr, CapeErr);
typedef int      (__STDCALL     *fct_qbus_on_done) (QBus, void* ptr, CapeErr);
typedef int      (__STDCALL     *fct_qbus_on_msg)  (QBus, void* ptr, QBusM qin, QBusM qout, CapeErr);
typedef void     (__STDCALL     *fct_qbus_on_rm)   (void* ptr);
typedef void     (__STDCALL     *fct_qbus_on_val)  (QBus, void* ptr, const CapeString ident, CapeUdc val);

//-----------------------------------------------------------------------------

                                                         /* register a method callback */
__CAPE_LIBEX   int                qbus_register          (QBus, const CapeString method, void* user_ptr, fct_qbus_on_msg, fct_qbus_on_rm, CapeErr);

                                                         /* initialize a request to a remote method */
__CAPE_LIBEX   int                qbus_send              (QBus, const CapeString module, const CapeString method, QBusM msg, void* user_ptr, fct_qbus_on_msg, CapeErr);

                                                         /* continue with another request to a remote method */
__CAPE_LIBEX   int                qbus_continue          (QBus, const CapeString module, const CapeString method, QBusM qin, void** p_user_ptr, fct_qbus_on_msg, CapeErr);

                                                         /* save the current state, run this before qbus_response */
__CAPE_LIBEX   int                qbus_save              (QBus, QBusM qin, CapeString* p_skey, CapeErr);

                                                         /* send back the response, use qbus_save before */
__CAPE_LIBEX   int                qbus_response          (QBus, const CapeString skey, QBusM msg, CapeErr);

                                                         /* subscribe to an identifier */
__CAPE_LIBEX   int                qbus_subscribe         (QBus, const CapeString module, const CapeString ident, void* user_ptr, fct_qbus_on_val, CapeErr);

                                                         /* unsubscribe from an identifier */
__CAPE_LIBEX   int                qbus_unsubscribe       (QBus, const CapeString module, const CapeString ident, CapeErr);

                                                         /* send value as an identifier */
__CAPE_LIBEX   void               qbus_next              (QBus, const CapeString ident, CapeUdc* p_val);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioContext     qbus_aio               (QBus);

__CAPE_LIBEX   CapeUdc            qbus_modules           (QBus);

__CAPE_LIBEX   QBusConfig         qbus_config            (QBus);

//-----------------------------------------------------------------------------

__CAPE_LIBEX  void                qbus_log_msg           (QBus, const CapeString remote, const CapeString message);

__CAPE_LIBEX  void                qbus_log_fmt           (QBus, const CapeString remote, const char* format, ...);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_instance          (const char* name, void* ptr, fct_qbus_on_init, fct_qbus_on_done, number_t argc, char *argv[]);

//-----------------------------------------------------------------------------

#endif
