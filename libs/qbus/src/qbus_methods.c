#include "qbus_methods.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusMethodItem_s
{
  void* user_ptr;
  fct_qbus_on_msg on_msg;
  
  CapeString skey;
  CapeString sender;
  
  CapeUdc rinfo;
  CapeString cid;
  
}; typedef struct QBusMethodItem_s* QBusMethodItem;

//-----------------------------------------------------------------------------

QBusMethodItem qbus_method_item_new (void* user_ptr, fct_qbus_on_msg on_msg, const CapeString saves_key, const CapeString sender, const CapeString cid, const CapeUdc rinfo)
{
  QBusMethodItem self = CAPE_NEW (struct QBusMethodItem_s);
  
  self->user_ptr = user_ptr;
  self->on_msg = on_msg;
  
  self->skey = cape_str_cp (saves_key);
  self->sender = cape_str_cp (sender);

  self->rinfo = cape_udc_cp (rinfo);
  self->cid = cape_str_cp (cid);
  
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

    cape_udc_del (&(self->rinfo));
    cape_str_del (&(self->cid));
    
    CAPE_DEL (p_self, struct QBusMethodItem_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_method_item_cid (QBusMethodItem self)
{
  return self->sender;
}

//-----------------------------------------------------------------------------

const CapeString qbus_method_item_skey (QBusMethodItem self)
{
  return self->skey;
}

//-----------------------------------------------------------------------------

struct QBusMethodSubItem_s
{
  void* user_ptr;
  fct_qbus_on_val on_val;
  
}; typedef struct QBusMethodSubItem_s* QBusMethodSubItem;

//-----------------------------------------------------------------------------

QBusMethodSubItem qbus_method_sub_item_new (void* user_ptr, fct_qbus_on_val on_val)
{
  QBusMethodSubItem self = CAPE_NEW (struct QBusMethodSubItem_s);
  
  self->user_ptr = user_ptr;
  self->on_val = on_val;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_method_sub_item_del (QBusMethodSubItem* p_self)
{
  if (*p_self)
  {
//    QBusMethodSubItem self = *p_self;
    
    
    CAPE_DEL (p_self, struct QBusMethodSubItem_s);
  }
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
  
  CapeMutex sub_mutex;
  CapeMap sub_items;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods__rpc_items__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethodItem h = val; qbus_method_item_del (&h);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods__sub_items__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethodSubItem h = val; qbus_method_sub_item_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusMethods qbus_methods_new (QBus qbus)
{
  QBusMethods self = CAPE_NEW (struct QBusMethods_s);
  
  // run a task maximum of 10 seconds
  self->queue = cape_queue_new (qbus_config_n (qbus_config (qbus), "queue_timeout", 10000));

  self->user_ptr = NULL;
  self->on_res = NULL;
  
  self->qbus = qbus;

  self->saves_mutex = cape_mutex_new ();
  self->methods = cape_map_new (cape_map__compare__s, qbus_methods__rpc_items__on_del, NULL);
  self->saves = cape_map_new (cape_map__compare__s, qbus_methods__rpc_items__on_del, NULL);
  
  self->sub_mutex = cape_mutex_new ();
  self->sub_items = cape_map_new (cape_map__compare__s, qbus_methods__sub_items__on_del, NULL);
  
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

    cape_map_del (&(self->sub_items));
    cape_mutex_del (&(self->sub_mutex));

    CAPE_DEL (p_self, struct QBusMethods_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_methods_init (QBusMethods self, number_t threads, void* user_ptr, fct_qbus_methods__on_res on_res, CapeErr err)
{
  self->user_ptr = user_ptr;
  self->on_res = on_res;

  cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "methods", "start queue with %lu threads", threads);
  
  return cape_queue_start (self->queue, (int)threads, err);
}

//-----------------------------------------------------------------------------

QBusMethodItem qbus_methods_load (QBusMethods self, const CapeString skey)
{
  QBusMethodItem mitem = NULL;
  
  cape_mutex_lock (self->saves_mutex);

  CapeMapNode n = cape_map_find (self->saves, (void*)skey);

  if (n)
  {
    mitem = cape_map_node_value (n);
    
    cape_map_node_set (n, NULL);

    cape_map_erase (self->saves, n);

 //   cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "load", "load skey = '%s'", skey);
  }
  else
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "load", "can't load skey = '%s'", skey);
  }
  
  cape_mutex_unlock (self->saves_mutex);
  
  return mitem;
}

//-----------------------------------------------------------------------------

const CapeString qbus_methods_save (QBusMethods self, void* user_ptr, fct_qbus_on_msg on_msg, const CapeString saves_key, const CapeString sender, CapeUdc rinfo, const CapeString cid)
{
  CapeString skey = cape_str_uuid ();
  
  QBusMethodItem mitem = qbus_method_item_new (user_ptr, on_msg, saves_key, sender, cid, rinfo);
  
  cape_mutex_lock (self->saves_mutex);
  
  cape_map_insert (self->saves, (void*)skey, (void*)mitem);

  cape_mutex_unlock (self->saves_mutex);
  
  //cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "save", "save skey = '%s' -> %p [skey: %s, sender: %s] {%s}", skey, mitem, saves_key, sender, cid);

  return skey;
}

//-----------------------------------------------------------------------------

int qbus_methods__rpc_add (QBusMethods self, const CapeString method, void* user_ptr, fct_qbus_on_msg on_msg, fct_qbus_on_rm on_rm, CapeErr err)
{
  cape_map_insert (self->methods, (void*)cape_str_cp (method), (void*)qbus_method_item_new (user_ptr, on_msg, NULL, NULL, NULL, NULL));
    
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

void qbus_methods_response (QBusMethods self, QBusMethodItem mitem, QBusM* p_msg, CapeErr err)
{
  if (self->on_res && mitem)
  {
    QBusM qout = *p_msg;
    
    // copy the error object to the message
    qout->err = cape_err_cp (err);
            
    if ((NULL == qout->rinfo) && mitem->rinfo)
    {
      // move rinfo
      qout->rinfo = cape_udc_mv (&(mitem->rinfo));
    }
    
    self->on_res (self->user_ptr, mitem, p_msg);
  }
  else
  {
    qbus_message_del (p_msg);

    //cape_log_msg (CAPE_LL_WARN, "QBUS", "on event", "no 'on_res' callback was set");
  }
}

//-----------------------------------------------------------------------------

void qbus_methods_send (QBusMethods self, const CapeString saves_key, CapeErr err)
{
  QBusMethodItem mitem = NULL;
  QBusM qout = qbus_message_new (NULL, NULL);
  
  if (saves_key)
  {
    mitem = qbus_methods_load (self, saves_key);
  }
  
  qbus_methods_response (self, mitem, &qout, err);
  
  qbus_method_item_del (&mitem);
  qbus_message_del (&qout);
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
    
    // check for special case continue
    if (res == CAPE_ERR_CONTINUE)
    {
      // do nothing, the response is handled on another event
    }
    else
    {
      if (res)
      {
        cape_log_fmt (CAPE_LL_WARN, "QBUS", "queue", "method returned an error [%lu]: %s", res, cape_err_text (err));
      }
      
      if (mctx->saves_key)
      {
        mitem = qbus_methods_load (mctx->self, mctx->saves_key);
      }
      
      // set the error object only in case there was an error
      qbus_methods_response (mctx->self, mitem, &qout, res ? err : NULL);
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
    qbus_message_del (p_qin);
    
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", method_name);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_methods_abort (QBusMethods self, const CapeString cid, const CapeString name)
{
  cape_mutex_lock (self->saves_mutex);
  
  {
    CapeMapCursor* cursor = cape_map_cursor_new (self->saves, CAPE_DIRECTION_FORW);

    while (cape_map_cursor_next (cursor))
    {
      QBusMethodItem mitem = cape_map_node_value (cursor->node);
      
      if (cape_str_equal (cid, mitem->cid))
      {
        // the mitem skey has the original the request context
        const CapeString saves_key = qbus_method_item_skey (mitem);

        QBusM qin = qbus_message_new (saves_key, NULL);
        
        cape_err_set_fmt (qbus_message_err_new (qin), CAPE_ERR_PROCESS_ABORT, "module [%s] has terminated", name);
        
        // continue with the user process
        qbus_methods_queue (self, mitem, &qin, saves_key);
        
        qbus_method_item_del (&mitem);
        
        cape_map_node_set (cursor->node, NULL);

        cape_map_cursor_erase (self->saves, cursor);
        
        qbus_message_del (&qin);
      }      
    }
    
    cape_map_cursor_del(&cursor);
  }
  
  cape_mutex_unlock (self->saves_mutex);
}

//-----------------------------------------------------------------------------

int qbus_methods__sub_add (QBusMethods self, const CapeString topic, void* user_ptr, fct_qbus_on_val on_val, CapeErr err)
{
  int res;
  
  cape_mutex_lock (self->sub_mutex);

  {
    CapeMapNode n = cape_map_find (self->sub_items, (void*)topic);
    if (n)
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.METHOD_ALREADY_EXISTS");
    }
    else
    {
      cape_map_insert (self->sub_items, (void*)cape_str_cp (topic), (void*)qbus_method_sub_item_new (user_ptr, on_val));
      
      res = CAPE_ERR_NONE;
    }
  }

  cape_mutex_unlock (self->sub_mutex);
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_methods__sub_rm (QBusMethods self, const CapeString topic)
{
  cape_mutex_lock (self->sub_mutex);

  {
    CapeMapNode n = cape_map_find (self->sub_items, (void*)topic);
    if (n)
    {
      cape_map_erase (self->sub_items, n);
    }
    else
    {

    }
  }

  cape_mutex_unlock (self->sub_mutex);
}

//-----------------------------------------------------------------------------

int qbus_methods__sub_run (QBusMethods self, const CapeString topic, CapeUdc* p_val, CapeErr err)
{
  int res;
  
  cape_mutex_lock (self->sub_mutex);

  {
    CapeMapNode n = cape_map_find (self->sub_items, (void*)topic);
    if (n)
    {
      // TODO: must be implemented
      
    }
    else
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.TOPIC_NOT_FOUND");
    }
  }

  cape_mutex_unlock (self->sub_mutex);
  
  // cleanup
  cape_udc_del (p_val);
  
  return res;
}

//-----------------------------------------------------------------------------
