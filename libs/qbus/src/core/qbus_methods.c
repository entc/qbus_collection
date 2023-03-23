#include "qbus_methods.h"
#include "qbus.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusMethodItem_s
{
  number_t type;
  CapeString name;
  
  void* user_ptr;  
  fct_qbus_onMessage user_fct;
  
  // for continue
  
  CapeString chain_key;
  CapeString chain_sender;
  
  CapeUdc rinfo;
};

//-----------------------------------------------------------------------------

QBusMethodItem qbus_methods_item_new (number_t type, const CapeString method, void* user_ptr, fct_qbus_onMessage user_fct)
{
  QBusMethodItem self = CAPE_NEW (struct QBusMethodItem_s);
  
  self->type = type;
  self->name = cape_str_cp (method);
  self->user_ptr = user_ptr;
  self->user_fct = user_fct;
  
  self->chain_key = NULL;
  self->chain_sender = NULL;
  self->rinfo = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_methods_item_del (QBusMethodItem* p_self)
{
  if (*p_self)
  {
    QBusMethodItem self = *p_self;

    cape_str_del (&(self->chain_key));
    cape_str_del (&(self->chain_sender));
    
    cape_udc_del (&(self->rinfo));
    
    cape_str_del (&(self->name));
   
    CAPE_DEL (p_self, struct QBusMethodItem_s);
  }
}

//-----------------------------------------------------------------------------

number_t qbus_methods_item_type (QBusMethodItem self)
{
  return self->type;
}

//-----------------------------------------------------------------------------

int qbus_methods_item__continue_chain (QBusMethodItem self, QBus qbus, QBusM qin, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  QBusM qout = NULL;
  
  if (qin->rinfo == NULL)
  {
    // transfer ownership
    qin->rinfo = self->rinfo;
    self->rinfo = NULL;
  }
  
  // create an empty output message
  qout = qbus_message_new (NULL, NULL);
  
  // correct chainkey and sender
  // this is important, if another continue was used
  cape_str_replace_cp (&(qin->chain_key), self->chain_key);
  cape_str_replace_cp (&(qin->sender), self->chain_sender);
  
  switch (self->user_fct (qbus, self->user_ptr, qin, qout, err))
  {
    case CAPE_ERR_CONTINUE:
    {
      break;
    }
    default:
    {
      // use the correct chain key
      cape_str_replace_cp (&(qout->chain_key), self->chain_key);
      
      // use the correct sender
      cape_str_replace_cp (&(qout->sender), self->chain_sender);
      
      // send the response
      qbus_response (qbus, self->chain_sender, qout, err);
      
      break;
    }
  }
  
  // cleanup
  qbus_message_del (&qout);
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_methods_item__call_response (QBusMethodItem self, QBus qbus, QBusM qin, QBusM qout, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->user_fct)
  {
    if (self->chain_key)
    {
      res = qbus_methods_item__continue_chain (self, qbus, qin, err);
    }
    else
    {
      if (self->rinfo)
      {
        // replace rinfo
        cape_udc_replace_mv (&(qin->rinfo), &(self->rinfo));
      }
      
      // call the original callback
      res = self->user_fct (qbus, self->user_ptr, qin, qout, err);
    }    
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_methods_item__call_request (QBusMethodItem self, QBus qbus, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  switch (self->type)
  {
    case QBUS_METHOD_TYPE__REQUEST:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call method '%s'", self->name);
      
      if (self->user_fct)
      {
        // call the original callback
        res = self->user_fct (qbus, self->user_ptr, qin, qout, err);
      }
      else
      {
        res = CAPE_ERR_NONE;
      }
      
      break;
    }
    default:
    {
      res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", self->name);
      break;
    }
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_methods_item_continue (QBusMethodItem self, CapeString* p_chain_key, CapeString* p_chain_sender, CapeUdc* p_rinfo)
{
  cape_str_replace_mv (&(self->chain_key), p_chain_key);
  cape_str_replace_mv (&(self->chain_sender), p_chain_sender);
  
  self->rinfo = cape_udc_mv (p_rinfo);
}

//-----------------------------------------------------------------------------

struct QBusMethods_s
{
  CapeMutex mutex;
  
  CapeMap methods;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods__methods__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethodItem h = val; qbus_methods_item_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusMethods qbus_methods_new ()
{
  QBusMethods self = CAPE_NEW (struct QBusMethods_s);
  
  self->mutex = cape_mutex_new ();
  self->methods = cape_map_new (NULL, qbus_methods__methods__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_methods_del (QBusMethods* p_self)
{
  if (*p_self)
  {
    QBusMethods self = *p_self;
    
    cape_map_del (&(self->methods));
    cape_mutex_del (&(self->mutex));
    
    CAPE_DEL (p_self, struct QBusMethods_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_methods_add (QBusMethods self, const CapeString method, void* user_ptr, fct_qbus_onMessage on_event, CapeErr err)
{
  int res;
  
  cape_mutex_lock (self->mutex);
  
  {
    CapeMapNode n = cape_map_find (self->methods, (void*)method);
    
    if (n)
    {
      res = cape_err_set (err, CAPE_ERR_OUT_OF_BOUNDS, "method was already added");
    }
    else
    {
      cape_map_insert (self->methods, (void*)cape_str_cp (method), (void*)qbus_methods_item_new (QBUS_METHOD_TYPE__REQUEST, method, user_ptr, on_event));
      
      res = CAPE_ERR_NONE;
    }
  }

  cape_mutex_unlock (self->mutex);

  return res;
}

//-----------------------------------------------------------------------------

QBusMethodItem qbus_methods_get (QBusMethods self, const CapeString method)
{
  QBusMethodItem ret;
  
  cape_mutex_lock (self->mutex);
  
  {
    CapeMapNode n = cape_map_find (self->methods, (void*)method);
    
    if (n)
    {
      ret = cape_map_node_value (n);
    }
    else
    {
      ret = NULL;
    }
  }
  
  cape_mutex_unlock (self->mutex);
  
  return ret;
}

//-----------------------------------------------------------------------------
