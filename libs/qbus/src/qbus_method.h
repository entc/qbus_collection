#ifndef __QBUS_METHOD__H
#define __QBUS_METHOD__H 1

// qbus includes
#include "qbus_types.h"

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

typedef int    (__STDCALL         *fct_qbus_onMessage)   (QBus, void* ptr, QBusM qin, QBusM qout, CapeErr);
typedef void   (__STDCALL         *fct_qbus_onRemoved)   (void* ptr);

//-----------------------------------------------------------------------------

struct QBusMethod_s; typedef struct QBusMethod_s* QBusMethod; // use a simple version

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusMethod         qbus_method_new               (const CapeString chainkey, void* user_ptr, fct_qbus_onMessage user_fct);

__CAPE_LIBEX   void               qbus_method_del               (QBusMethod*);

__CAPE_LIBEX   int                qbus_method_run               (QBusMethod, QBus, QBusM qin, QBusM qout, CapeErr);

//-----------------------------------------------------------------------------

#endif 
