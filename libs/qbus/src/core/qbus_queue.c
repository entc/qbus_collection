#include "qbus_queue.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

QBusQueueItem qbus_queue_item_new (QBusM msg, const CapeString module, const CapeString method, void* user_ptr, fct_qbus_onMessage user_fct)
{
  QBusQueueItem self = CAPE_NEW (struct QBusQueueItem_s);
  
  self->msg = qbus_message_data_mv (msg);

  self->module = cape_str_cp (module);
  self->method = cape_str_cp (method);
  
  self->user_ptr = user_ptr;
  self->user_fct = user_fct;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_queue_item_del (QBusQueueItem* p_self)
{
  if (*p_self)
  {
    QBusQueueItem self = *p_self;
    
    cape_str_del (&(self->method));
    cape_str_del (&(self->module));
    
    qbus_message_del (&(self->msg));
    
    CAPE_DEL (p_self, struct QBusQueueItem_s);
  }
}

//-----------------------------------------------------------------------------

struct QBusQueueProcessItem_s
{
  QBusPvdConnection conn;     // reference
  QBusQueueItem item;         // owned
  
  void* qbus_ptr;
  fct_qbus__on_queue qbus_fct;
  
}; typedef struct QBusQueueProcessItem_s* QBusQueueProcessItem;

//-----------------------------------------------------------------------------

struct QBusQueue_s
{
  CapeQueue queue;
};

//-----------------------------------------------------------------------------

QBusQueue qbus_queue_new ()
{
  QBusQueue self = CAPE_NEW (struct QBusQueue_s);
  
  self->queue = cape_queue_new (5000);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_queue_del (QBusQueue* p_self)
{
  if (*p_self)
  {
    QBusQueue self = *p_self;
    
    cape_queue_del (&(self->queue));
    
    CAPE_DEL (p_self, struct QBusQueue_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_queue_init (QBusQueue self, number_t threads, CapeErr err)
{
  return cape_queue_start (self->queue, threads, err);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_queue_add__on_process (void* ptr, number_t pos, number_t queue_size)
{
  QBusQueueProcessItem item = ptr;
  
  item->qbus_fct (item->qbus_ptr, item->conn, item->item);

  qbus_queue_item_del (&(item->item));
  
  CAPE_DEL (&item, struct QBusQueueProcessItem_s);
}

//-----------------------------------------------------------------------------

void qbus_queue_add (QBusQueue self, QBusPvdConnection conn, QBusQueueItem* p_qitem, void* qbus_ptr, fct_qbus__on_queue qbus_fct)
{
  QBusQueueProcessItem item = CAPE_NEW (struct QBusQueueProcessItem_s);
  
  item->conn = conn;
  
  item->item = *p_qitem;
  *p_qitem = NULL;
  
  item->qbus_ptr = qbus_ptr;
  item->qbus_fct = qbus_fct;
  
  cape_queue_add (self->queue, NULL, qbus_queue_add__on_process, NULL, item, 0);
}

//-----------------------------------------------------------------------------
