#ifndef __QBUS_TL2__H
#define __QBUS_TL2__H 1

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusEngine_s; typedef struct QBusEngine_s* QBusEngine; // use a simple version

//-----------------------------------------------------------------------------

typedef void          (__STDCALL *fct_qbus_engine__on_connect)       (void* user_ptr, void* node);

typedef void          (__STDCALL *fct_qbus_engine__on_disconnect)    (void* user_ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusEngine         qbus_engine_new               ();

__CAPE_LIBEX   void               qbus_engine_del               (QBusEngine*);

__CAPE_LIBEX   int                qbus_engine_init              (QBusEngine, void* user_ptr, fct_qbus_engine__on_connect, fct_qbus_engine__on_disconnect, CapeErr err);

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------

#endif
