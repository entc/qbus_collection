#include "qbus_methods.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <sys/cape_queue.h>

struct QBusMethodItem_s
{
  void* user_ptr;
  fct_qbus_on_msg on_msg;
  
}; typedef struct QBusMethodItem_s* QBusMethodItem;

//-----------------------------------------------------------------------------

struct QBusMethods_s
{
  CapeQueue queue;
  CapeMap methods;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods__methods__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethodItem h = val;
  }
}

//-----------------------------------------------------------------------------

QBusMethods qbus_methods_new ()
{
  QBusMethods self = CAPE_NEW (struct QBusMethods_s);
  
  self->queue = cape_queue_new (10000);     // run a task maximum of 10 seconds
  self->methods = cape_map_new (cape_map__compare__s, qbus_methods__methods__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_methods_del (QBusMethods* p_self)
{
  if (*p_self)
  {
    QBusMethods self = *p_self;
    
    cape_queue_del (&(self->queue));
    cape_map_del (&(self->methods));
    
    CAPE_DEL (p_self, struct QBusMethods_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_methods_init (QBusMethods self, number_t threads, CapeErr err)
{
  return cape_queue_start (self->queue, (int)threads, err);
}

//-----------------------------------------------------------------------------

int qbus_methods_add (QBusMethods self, const CapeString method, void* user_ptr, fct_qbus_on_msg on_msg, fct_qbus_on_rm on_rm, CapeErr err)
{
  QBusMethodItem mitem = CAPE_NEW (struct QBusMethodItem_s);
  
  mitem->user_ptr = user_ptr;
  mitem->on_msg = on_msg;
  
  cape_map_insert (self->methods, (void*)cape_str_cp (method), (void*)mitem);
    
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods__queue__on_event (void* user_ptr, number_t pos, number_t queue_size)
{
  QBusMethodItem mitem = user_ptr;
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "on event", "queue task started, size = %lu", queue_size);
  
  if (queue_size > 20)
  {
    
    
  }
  
}

//-----------------------------------------------------------------------------

int qbus_methods_run (QBusMethods self, const CapeString method_name, CapeErr err)
{
  int res;
  
  // seek the method
  CapeMapNode n = cape_map_find (self->methods, (void*)method_name);
  
  if (n)
  {
    cape_queue_add (self->queue, NULL, qbus_methods__queue__on_event, NULL, NULL, cape_map_node_value (n), 0);
  }
  else
  {
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", method_name);
  }
  
  return res;
}

//-----------------------------------------------------------------------------
