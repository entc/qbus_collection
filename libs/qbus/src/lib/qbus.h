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

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                qbus_send              (QBus, const CapeString module, const CapeString method, QBusM msg, void* user_ptr, fct_qbus_on_msg, CapeErr);

__CAPE_LIBEX   CapeAioContext     qbus_aio               (QBus);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_instance          (const char* name, void* ptr, fct_qbus_on_init, fct_qbus_on_done, int argc, char *argv[]);

//-----------------------------------------------------------------------------

#endif
