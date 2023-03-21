#ifndef __QBUS__METHODS__H
#define __QBUS__METHODS__H 1

#include "qbus_route.h"
#include "qbus_message.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusMethodItem_s; typedef struct QBusMethodItem_s* QBusMethodItem;
struct QBusMethods_s; typedef struct QBusMethods_s* QBusMethods;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusMethods        qbus_methods_new             ();

__CAPE_LOCAL   void               qbus_methods_del             (QBusMethods*);

__CAPE_LOCAL   QBusMethodItem     qbus_methods_get             (QBusMethods, const CapeString method);

//-----------------------------------------------------------------------------

#endif
