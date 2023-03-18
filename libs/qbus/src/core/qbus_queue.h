#ifndef __QBUS__QUEUE__H
#define __QBUS__QUEUE__H 1

#include "qbus_message.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusQueue_s; typedef struct QBusQueue_s* QBusQueue;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusQueue          qbus_queue_new               ();

__CAPE_LOCAL   void               qbus_queue_del               (QBusQueue*);

__CAPE_LOCAL   int                qbus_queue_init              (QBusQueue, number_t threads, CapeErr);

//-----------------------------------------------------------------------------

#endif
