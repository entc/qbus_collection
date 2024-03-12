#ifndef __QBUS_STORAGE__H
#define __QBUS_STORAGE__H 1

// qbus includes
#include "qbus_method.h"

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusStorage_s; typedef struct QBusStorage_s* QBusStorage; // use a simple version

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusStorage        qbus_storage_new              ();

__CAPE_LIBEX   void               qbus_storage_del              (QBusStorage*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                qbus_storage_add              (QBusStorage, const CapeString method, void* ptr, fct_qbus_onMessage on_msg, CapeErr err);

__CAPE_LIBEX   QBusMethod         qbus_storage_get              (QBusStorage, const CapeString method);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_storage_queue_add        (QBusStorage, QBusMethod);

//-----------------------------------------------------------------------------

#endif 
