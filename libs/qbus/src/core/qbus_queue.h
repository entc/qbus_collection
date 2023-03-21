#ifndef __QBUS__QUEUE__H
#define __QBUS__QUEUE__H 1

#include "qbus_route.h"
#include "qbus_message.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusQueueItem_s
{
  
  QBusM msg;
  
  CapeString module;
  CapeString method;
  
  void* user_ptr;
  fct_qbus_onMessage user_fct;
  
}; typedef struct QBusQueueItem_s* QBusQueueItem;

//-----------------------------------------------------------------------------

typedef void           (__STDCALL *fct_qbus__on_queue)        (void* qbus_ptr, QBusPvdConnection conn, QBusQueueItem);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusQueueItem      qbus_queue_item_new (QBusM msg, const CapeString module, const CapeString method, void* user_ptr, fct_qbus_onMessage user_fct);

__CAPE_LOCAL   void               qbus_queue_item_del (QBusQueueItem*);

//-----------------------------------------------------------------------------

struct QBusQueue_s; typedef struct QBusQueue_s* QBusQueue;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusQueue          qbus_queue_new               ();

__CAPE_LOCAL   void               qbus_queue_del               (QBusQueue*);

__CAPE_LOCAL   int                qbus_queue_init              (QBusQueue, number_t threads, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_queue_add               (QBusQueue, QBusPvdConnection, QBusQueueItem* qitem, void* qbus_ptr, fct_qbus__on_queue qbus_fct);

//-----------------------------------------------------------------------------

#endif
