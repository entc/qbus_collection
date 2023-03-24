#include "qbus_methods.h"
#include "qbus.h"
#include "qbus_chain.h"
#include "qbus_engines.h"

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
  
  QBusEngines engines;   // reference
  QBusQueue queue;       // reference
  
  QBusChain chain;
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

QBusMethods qbus_methods_new (QBusEngines engines, QBusQueue queue)
{
  QBusMethods self = CAPE_NEW (struct QBusMethods_s);
  
  self->engines = engines;
  self->queue = queue;
  
  self->mutex = cape_mutex_new ();
  self->methods = cape_map_new (NULL, qbus_methods__methods__on_del, NULL);
  
  self->chain = qbus_chain_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_methods_del (QBusMethods* p_self)
{
  if (*p_self)
  {
    QBusMethods self = *p_self;
    
    qbus_chain_del (&(self->chain));
    
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

void qbus_methods_handle_response (QBusMethods self, QBus qbus, QBusM msg)
{
  if (msg->chain_key)
  {
    QBusMethodItem mitem = qbus_chain_ext (self->chain, msg->chain_key);
    
    if (mitem)
    {
      switch (qbus_methods_item_type (mitem))
      {
        case QBUS_METHOD_TYPE__RESPONSE:
        {
          CapeErr err = cape_err_new ();
          
          qbus_methods_item__call_response (mitem, qbus, msg, NULL, err);
          
          cape_err_del (&err);
          
          break;
        }
      }
      
      qbus_methods_item_del (&mitem);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "QBUS", "response", "chain key was not found in local response");
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_methods_proc_request (QBusMethods self, QBus qbus, QBusQueueItem qitem, const CapeString sender)
{
  int res;
  QBusMethodItem mitem;
  
  // local objects
  CapeErr err = cape_err_new ();
  QBusM qout = qbus_message_new (NULL, NULL);
  
  CapeString last_chain_key = cape_str_mv (&(qitem->msg->chain_key));
  CapeString last_sender = cape_str_mv (&(qitem->msg->sender));
  
  CapeString next_chain_key = cape_str_uuid ();
  
  // set next chain key
  qitem->msg->chain_key = cape_str_cp (next_chain_key);
  qitem->msg->sender = cape_str_cp (sender);
  
  // set default message type
  qout->mtype = QBUS_MTYPE_JSON;
  
  // try to find the method stored in route
  mitem = qbus_methods_get (self, qitem->method);
  
  if (mitem == NULL)
  {
    goto exit_and_cleanup;
  }
  
  {
    QBusMethodItem mitem = qbus_methods_item_new (QBUS_METHOD_TYPE__RESPONSE, qitem->method, qitem->user_ptr, qitem->user_fct);
    
    {
      CapeString last_chain_key_copy = cape_str_cp (last_chain_key);
      CapeString last_sender_copy = cape_str_cp (last_sender);
      
      CapeUdc rinfo_copy = cape_udc_cp (qitem->msg->rinfo);
      
      qbus_methods_item_continue (mitem, &last_chain_key_copy, &last_sender_copy, &rinfo_copy);

      cape_udc_del (&rinfo_copy);
      
      cape_str_del (&last_sender_copy);
      cape_str_del (&last_chain_key_copy);
    }
    
    qbus_chain_add (self->chain, next_chain_key, &mitem);
  }
  
  res = qbus_methods_item__call_request (mitem, qbus, qitem->msg, qout, err);
  
  switch (res)
  {
    case CAPE_ERR_CONTINUE:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call returned a continued state");
      
      // this request shall not continue and another request was created instead
      // we need to add the callback and ptr to the chains list, for response
      // -> don't consider qout
      
      break;
    }
    default:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call returned a terminated state");
      
      if (res)
      {
        // tranfer ownership
        qout->err = err;
        
        // create an empty error object
        err = cape_err_new ();
      }
      
      // correct chain parameters
      cape_str_replace_cp (&(qout->chain_key), last_chain_key);
      cape_str_replace_cp (&(qout->sender), last_sender);
      
      // add rinfo
      cape_udc_replace_cp (&(qout->rinfo), qitem->msg->rinfo);
      
      {
        // create a method object to re-use existing functionality
        QBusMethodItem qmeth_next = qbus_methods_item_new (QBUS_METHOD_TYPE__RESPONSE, NULL, qitem->user_ptr, qitem->user_fct);
        
        // provide the last chain key to handle the return chain traversal path
        qbus_methods_item_continue (qmeth_next, &last_chain_key, &last_sender, &(qitem->msg->rinfo));
        
        // this requests ends here, now send the results back
        qbus_methods_item__call_response (qmeth_next, qbus, qout, NULL, err);
        
        qbus_methods_item_del (&qmeth_next);
      }
      
      break;
    }
  }
  
  exit_and_cleanup:
  
  cape_str_del (&next_chain_key);
  
  cape_str_replace_mv (&(qitem->msg->chain_key), &last_chain_key);
  cape_str_replace_mv (&(qitem->msg->sender), &last_sender);
  
  qbus_message_del (&qout);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qbus_methods_call (QBusMethods self, QBusFrame frame, QBusPvdConnection conn)
{
  
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods_recv__process_response (void* qbus_ptr, QBusPvdConnection conn, QBusQueueItem qitem)
{
  QBusMethods self = qbus_ptr;
  
  CapeErr err = cape_err_new ();
  
  /*
  // convert the frame content into the input message (expensive)
  QBusM qin = qbus_frame_qin (*p_frame);
  
  
  
  qbus_methods_item__call_response (qmeth, self->qbus, self, qin, NULL, err);
  
  qbus_methods_item__call_response (QBusMethodItem self, QBus qbus, qin, NULL, err);
  
  qbus_message_del (&qin);
  */

  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qbus_methods_recv_response (QBusMethods self, QBusFrame* p_frame, QBusPvdConnection conn)
{
  QBusFrame frame = *p_frame;
  
  if (frame->chain_key)
  {
    QBusMethodItem mitem = qbus_chain_ext (self->chain, frame->chain_key);
      
    if (mitem)
    {
      switch (qbus_methods_item_type (mitem))
      {
        case QBUS_METHOD_TYPE__REQUEST:
        {
          // this should not happen
          
          break;
        }
        case QBUS_METHOD_TYPE__RESPONSE:
        {
          QBusQueueItem item = qbus_queue_item_new (NULL, NULL, NULL, mitem->user_ptr, mitem->user_fct);
          
          // add to queue as local processing
          qbus_queue_add (self->queue, conn, &item, self, qbus_methods_recv__process_response);
          
          break;
        }
        case QBUS_METHOD_TYPE__FORWARD:
        {
    //      qbus_route_on_msg_forward (self, p_frame, qbus_method_ptr (qmeth));
          
          break;
        }
        case QBUS_METHOD_TYPE__METHODS:
        {
    //      qbus_route_on_msg_methods (self, p_frame, qbus_method_ptr (qmeth));
          
          break;
        }
      }
        
      qbus_methods_item_del (&mitem);
    }
    else
    {
      
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_methods_recv_methods (QBusMethods self, QBusPvdConnection conn)
{
  
}

//-----------------------------------------------------------------------------

void qbus_methods_recv_forward (QBusMethods self, QBusFrame frame, QBusPvdConnection conn, const CapeString sender)
{
  CapeString forward_chain_key = cape_str_uuid();
  CapeString forward_sender = cape_str_cp (sender);
  
  {
    QBusMethodItem mitem = qbus_methods_item_new (QBUS_METHOD_TYPE__FORWARD, NULL, NULL, NULL);
    
    // extract chain key and sender
    qbus_methods_item_continue (mitem, &(frame->chain_key), &(frame->sender), NULL);
    
    qbus_chain_add (self->chain, forward_chain_key, &mitem);
  }
  
  qbus_frame_set_chainkey (frame, &forward_chain_key);
  qbus_frame_set_sender (frame, &forward_sender);
  
  // finally send the frame
  qbus_engines__send (self->engines, frame, conn);
}

//-----------------------------------------------------------------------------

void qbus_methods_send_request (QBusMethods self, QBusPvdConnection conn, QBusQueueItem qitem, const CapeString sender, int cont)
{
  // create a new frame
  QBusFrame frame = qbus_frame_new ();
  
  // local objects
  CapeString next_chainkey = cape_str_uuid();
  
  // add default content
  qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_REQ, next_chainkey, qitem->module, qitem->method, sender);
  
  // add message content
  qitem->msg->rinfo = qbus_frame_set_qmsg (frame, qitem->msg, NULL);
  
  {
    QBusMethodItem mitem = qbus_methods_item_new (QBUS_METHOD_TYPE__RESPONSE, qitem->method, qitem->user_ptr, qitem->user_fct);

    qbus_methods_item_continue (mitem, cont ? &(qitem->msg->chain_key): NULL, &(qitem->msg->sender), &(qitem->msg->rinfo));
    
    qbus_chain_add (self->chain, next_chainkey, &mitem);
  }
    
  // finally send the frame
  qbus_engines__send (self->engines, frame, conn);
  
  qbus_frame_del (&frame);
}

//-----------------------------------------------------------------------------
 
void qbus_methods_send_methods (QBusMethods self, QBusPvdConnection conn, const CapeString module, const CapeString sender, void* user_ptr, fct_qbus_on_methods user_fct)
{
  // create a new frame
  QBusFrame frame = qbus_frame_new ();
  
  // create a new chain key
  CapeString next_chainkey = cape_str_uuid();
  
  {
    QBusMethodItem mitem = qbus_methods_item_new (QBUS_METHOD_TYPE__METHODS, NULL, user_ptr, user_fct);
    
    qbus_chain_add (self->chain, next_chainkey, &mitem);
  }
  
  qbus_frame_set (frame, QBUS_FRAME_TYPE_METHODS, next_chainkey, module, NULL, sender);
  
  // finally send the frame
  qbus_engines__send (self->engines, frame, conn);

  qbus_frame_del (&frame);
}

//-----------------------------------------------------------------------------
