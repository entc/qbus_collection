#include "qbus_chain.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusChain_s
{
  CapeMutex mutex;
  
  CapeMap chain_items;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_chain__item__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusChain qbus_chain_new ()
{
  QBusChain self = CAPE_NEW (struct QBusChain_s);
  
  self->mutex = cape_mutex_new ();
  self->chain_items = cape_map_new (NULL, qbus_chain__item__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_chain_del (QBusChain* p_self)
{
  if (*p_self)
  {
    QBusChain self = *p_self;
    
    cape_map_del (&(self->chain_items));
    cape_mutex_del (&(self->mutex));
    
    CAPE_DEL (p_self, struct QBusChain_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_chain_add__cp (QBusChain self, const CapeString chainkey, QBusMethodItem* p_mitem)
{
  cape_mutex_lock (self->mutex);

  cape_map_insert (self->chain_items, (void*)cape_str_cp (chainkey), (void*)*p_mitem);
  *p_mitem = NULL;
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

void qbus_chain_add__mv (QBusChain self, CapeString* p_chainkey, QBusMethodItem* p_mitem)
{
  cape_mutex_lock (self->mutex);
  
  cape_map_insert (self->chain_items, (void*)cape_str_mv (p_chainkey), (void*)*p_mitem);
  *p_mitem = NULL;

  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

QBusMethodItem qbus_chain_ext (QBusChain self, const CapeString chainkey)
{
  QBusMethodItem ret = NULL;

  if (chainkey)
  {
    cape_mutex_lock (self->mutex);
    
    {
      CapeMapNode n = cape_map_find (self->chain_items, (void*)chainkey);
      
      if (n)
      {
        n = cape_map_extract (self->chain_items, n);
        
        ret = cape_map_node_value (n);
        
        cape_map_del_node (self->chain_items, &n);
      }
    }
    
    cape_mutex_unlock (self->mutex);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_chain_all (QBusChain self, void* user_ptr, fct_qbus_chain__on_item on_item)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeMapCursor* cursor = cape_map_cursor_new (self->chain_items, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusMethodItem item = cape_map_node_value (cursor->node);
      
      // call the callback
      // if item is NULL we can remove this entry
      on_item (user_ptr, &item);
      
      if (item == NULL)
      {
        CapeMapNode n = cape_map_cursor_extract (self->chain_items, cursor);
       
        cape_map_del_node (self->chain_items, &n);
      }
    }
    
    cape_map_cursor_del (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------