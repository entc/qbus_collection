#include "qbus_methods.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <sys/cape_mutex.h>

struct QBusMethodItem_s
{
  void* user_ptr;
  fct_qbus_on_msg on_msg;
  
  CapeString skey;
  CapeString sender;
  
}; typedef struct QBusMethodItem_s* QBusMethodItem;

//-----------------------------------------------------------------------------

QBusMethodItem qbus_method_item_new (void* user_ptr, fct_qbus_on_msg on_msg, const CapeString saves_key, const CapeString sender)
{
  QBusMethodItem self = CAPE_NEW (struct QBusMethodItem_s);
  
  self->user_ptr = user_ptr;
  self->on_msg = on_msg;
  
  self->skey = cape_str_cp (saves_key);
  self->sender = cape_str_cp (sender);

  return self;
}

//-----------------------------------------------------------------------------

void qbus_method_item_del (QBusMethodItem* p_self)
{
  if (*p_self)
  {
    QBusMethodItem self = *p_self;
    
    cape_str_del (&(self->skey));
    cape_str_del (&(self->sender));

    CAPE_DEL (p_self, struct QBusMethodItem_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_method_item_sender (QBusMethodItem self)
{
  return self->sender;
}

//-----------------------------------------------------------------------------

const CapeString qbus_method_item_skey (QBusMethodItem self)
{
  return self->skey;
}

//-----------------------------------------------------------------------------

struct QBusMethodCtx_s
{
  QBusM qin;
  QBus qbus;
  QBusMethods self;
  
  CapeString saves_key;
  
  void* on_msg_user_ptr;
  fct_qbus_on_msg on_msg;

  void* on_res_user_ptr;
  fct_qbus_methods__on_res on_res;

}; typedef struct QBusMethodCtx_s* QBusMethodCtx;

//-----------------------------------------------------------------------------

void qbus_method_ctx_del (QBusMethodCtx* p_self)
{
  if (*p_self)
  {
    QBusMethodCtx self = *p_self;
    
    qbus_message_del (&(self->qin));
    cape_str_del (&(self->saves_key));

    CAPE_DEL (p_self, struct QBusMethodCtx_s);
  }
}

//-----------------------------------------------------------------------------

struct QBusMethods_s
{
  CapeQueue queue;
  CapeMap methods;
  CapeMap saves;
  
  QBus qbus;            // reference
  
  CapeMutex saves_mutex;
  
  void* user_ptr;
  fct_qbus_methods__on_res on_res;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods__methods__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethodItem h = val; qbus_method_item_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusMethods qbus_methods_new (QBus qbus)
{
  QBusMethods self = CAPE_NEW (struct QBusMethods_s);
  
  self->queue = cape_queue_new (10000);     // run a task maximum of 10 seconds
  self->methods = cape_map_new (cape_map__compare__s, qbus_methods__methods__on_del, NULL);
  self->saves = cape_map_new (cape_map__compare__s, qbus_methods__methods__on_del, NULL);

  self->saves_mutex = cape_mutex_new ();
    
  self->user_ptr = NULL;
  self->on_res = NULL;
  
  self->qbus = qbus;

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
    
    // for debug output
    {
//      cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "methods del", "found saved mitems = %lu", cape_map_size (self->saves));
    }
    
    cape_map_del (&(self->saves));
    cape_mutex_del (&(self->saves_mutex));
    
    CAPE_DEL (p_self, struct QBusMethods_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_methods_init (QBusMethods self, number_t threads, void* user_ptr, fct_qbus_methods__on_res on_res, CapeErr err)
{
  self->user_ptr = user_ptr;
  self->on_res = on_res;

  return cape_queue_start (self->queue, (int)threads, err);
}

//-----------------------------------------------------------------------------

QBusMethodItem qbus_methods_load (QBusMethods self, const CapeString save_key)
{
  QBusMethodItem mitem = NULL;
  
  cape_mutex_lock (self->saves_mutex);

  CapeMapNode n = cape_map_find (self->saves, (void*)save_key);

  if (n)
  {
    mitem = cape_map_node_value (n);
    
    cape_map_node_set (n, NULL);

    cape_map_erase (self->saves, n);
  }
  
  cape_mutex_unlock (self->saves_mutex);

  //cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "load", "load skey = '%s'", save_key);
  
  return mitem;
}

//-----------------------------------------------------------------------------

const CapeString qbus_methods_save (QBusMethods self, void* user_ptr, fct_qbus_on_msg on_msg, const CapeString saves_key, const CapeString sender)
{
  CapeString save_key = cape_str_uuid ();
  
  QBusMethodItem mitem = qbus_method_item_new (user_ptr, on_msg, saves_key, sender);
  
  cape_mutex_lock (self->saves_mutex);
  
  cape_map_insert (self->saves, (void*)save_key, (void*)mitem);

  cape_mutex_unlock (self->saves_mutex);
  
  //cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "save", "save skey = '%s' | on_msg = %p | sender = %s", save_key, on_msg, sender);

  return save_key;
}

//-----------------------------------------------------------------------------

int qbus_methods_add (QBusMethods self, const CapeString method, void* user_ptr, fct_qbus_on_msg on_msg, fct_qbus_on_rm on_rm, CapeErr err)
{
  cape_map_insert (self->methods, (void*)cape_str_cp (method), (void*)qbus_method_item_new (user_ptr, on_msg, NULL, NULL));
    
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods__queue__on_event (void* user_ptr, number_t pos, number_t queue_size);

//-----------------------------------------------------------------------------

void qbus_methods_queue (QBusMethods self, QBusMethodItem mitem, QBusM* p_qin, const CapeString skey)
{
  if (mitem->on_msg)
  {
    // create chain object
    QBusMethodCtx mctx = CAPE_NEW (struct QBusMethodCtx_s);
    
    mctx->on_msg_user_ptr = mitem->user_ptr;
    mctx->on_msg = mitem->on_msg;
    
    mctx->on_res_user_ptr = self->user_ptr;
    mctx->on_res = self->on_res;
    
    mctx->qin = *p_qin;
    *p_qin = NULL;

    mctx->qbus = self->qbus;
    mctx->self = self;
    
    mctx->saves_key = cape_str_cp (skey);
   
    cape_queue_add (self->queue, NULL, qbus_methods__queue__on_event, NULL, NULL, mctx, 0);
  }
  else
  {
    qbus_message_del (p_qin);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods__queue__on_event (void* user_ptr, number_t pos, number_t queue_size)
{
  QBusMethodCtx mctx = user_ptr;
  
  //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "on event", "queue task started, size = %lu", queue_size);
  
  if (queue_size > 20)
  {
    
    
  }
  
  {
    CapeErr err = cape_err_new ();
    QBusM qout = qbus_message_new (NULL, NULL);
    
    QBusMethodItem mitem = NULL;
    
    int res = mctx->on_msg (mctx->qbus, mctx->on_msg_user_ptr, mctx->qin, qout, err);

    if (mctx->saves_key)
    {
      mitem = qbus_methods_load (mctx->self, mctx->saves_key);
    }
    
    if (res == CAPE_ERR_CONTINUE)
    {
      
    }
    else
    {
      if (mctx->on_res && mitem)
      {
        // transfer err
        qout->err = err;
        err = NULL;
        
        mctx->on_res (mctx->on_res_user_ptr, mitem, &qout);
      }
      else
      {
        cape_log_msg (CAPE_LL_WARN, "QBUS", "on event", "no 'on_res' callback was set");
      }
    }
    
    qbus_method_item_del (&mitem);

    qbus_message_del (&qout);
    cape_err_del (&err);
  }
  
  qbus_method_ctx_del (&mctx);
}

//-----------------------------------------------------------------------------

int qbus_methods_run (QBusMethods self, const CapeString method_name, const CapeString saves_key, QBusM* p_qin, CapeErr err)
{
  int res;
  
  // seek the method
  CapeMapNode n = cape_map_find (self->methods, (void*)method_name);
  
  if (n)
  {
    qbus_methods_queue (self, cape_map_node_value (n), p_qin, saves_key);
    
    res = CAPE_ERR_NONE;
  }
  else
  {
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", method_name);
  }
  
  return res;
}

//-----------------------------------------------------------------------------
