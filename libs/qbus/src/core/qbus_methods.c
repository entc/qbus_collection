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

  fct_qbus_onMessage on_message;
  fct_qbus_on_methods on_methods;
  
  // for continue
  
  CapeString chain_key;
  CapeString chain_sender;
  
  CapeUdc rinfo;
  
};

//-----------------------------------------------------------------------------

QBusMethodItem qbus_methods_item_new (number_t type, const CapeString method, void* user_ptr, fct_qbus_onMessage on_message, fct_qbus_on_methods on_methods)
{
  QBusMethodItem self = CAPE_NEW (struct QBusMethodItem_s);
  
  self->type = type;
  self->name = cape_str_cp (method);
  self->user_ptr = user_ptr;
  
  self->on_message = on_message;
  self->on_methods = on_methods;
  
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
  
  switch (self->on_message (qbus, self->user_ptr, qin, qout, err))
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
  
  if (self->on_message)
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
      res = self->on_message (qbus, self->user_ptr, qin, qout, err);
    }    
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_methods_item__call_methods (QBusMethodItem self, QBus qbus, CapeUdc methods, CapeErr err)
{
  if (self->on_methods)
  {
    // call the original callback
    self->on_methods (qbus, self->user_ptr, methods, err);
  }
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
      
      if (self->on_message)
      {
        // call the original callback
        res = self->on_message (qbus, self->user_ptr, qin, qout, err);
      }
      else
      {
        res = CAPE_ERR_NONE;
      }
      
      break;
    }
    default:
    {
      res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "unsupported type [%lu]", self->type);
      break;
    }
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_methods_item_continue (QBusMethodItem self, CapeString* p_chain_key, CapeString* p_chain_sender, CapeUdc* p_rinfo)
{
  if (p_chain_key)
  {
    cape_str_replace_mv (&(self->chain_key), p_chain_key);
  }

  cape_str_replace_mv (&(self->chain_sender), p_chain_sender);
  
  self->rinfo = cape_udc_mv (p_rinfo);
}

//-----------------------------------------------------------------------------

struct QBusMethods_s
{
  CapeMutex mutex;
  
  CapeMap methods;
  
  CapeQueue queue;       // reference
  QBusEngines engines;   // reference
  QBus qbus;             // reference
    
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

QBusMethods qbus_methods_new (QBusEngines engines, QBus qbus, CapeQueue queue)
{
  QBusMethods self = CAPE_NEW (struct QBusMethods_s);
  
  self->qbus = qbus;
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
      cape_map_insert (self->methods, (void*)cape_str_cp (method), (void*)qbus_methods_item_new (QBUS_METHOD_TYPE__REQUEST, method, user_ptr, on_event, NULL));
      
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

CapeUdc qbus_methods_generate_methods_list (QBusMethods self)
{
  // encode methods into list
  CapeUdc method_list = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  cape_mutex_lock (self->mutex);

  // iterate through all methods
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->methods, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      cape_udc_add_s_cp (method_list, NULL, cape_map_node_key (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  cape_mutex_unlock (self->mutex);
  
  return method_list;
}

//-----------------------------------------------------------------------------

void qbus_methods_handle_response (QBusMethods self, QBusM msg)
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
          
          qbus_methods_item__call_response (mitem, self->qbus, msg, NULL, err);
          
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

struct QBusMethodProcessRecvItem
{
  QBus qbus;                  // reference
  QBusEngines engines;        // reference
  QBusPvdConnection conn;     // reference
  
  QBusMethodItem mitem;       // owned
  QBusFrame frame;            // owned
  CapeString sender;
};

//-----------------------------------------------------------------------------

struct QBusMethodProcessRecvItem* qbus_methods_recv_item_new (QBus qbus, QBusEngines engines, QBusPvdConnection conn, QBusMethodItem* p_mitem, QBusFrame* p_frame, const CapeString sender)
{
  struct QBusMethodProcessRecvItem* self = CAPE_NEW (struct QBusMethodProcessRecvItem);
  
  self->qbus = qbus;
  self->engines = engines;
  self->conn = conn;
  
  // transfer ownership
  self->mitem = *p_mitem;
  *p_mitem = NULL;
  
  // transfer ownership
  self->frame = *p_frame;
  *p_frame = NULL;
  
  self->sender = cape_str_cp (sender);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_methods_recv_item_del (struct QBusMethodProcessRecvItem** p_self)
{
  if (*p_self)
  {
    struct QBusMethodProcessRecvItem* self = *p_self;
    
    cape_str_del (&(self->sender));
    qbus_methods_item_del (&(self->mitem));
    qbus_frame_del (&(self->frame));
    
    CAPE_DEL(p_self, struct QBusMethodProcessRecvItem);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods_recv__process_response (void* ptr, number_t pos, number_t queue_size)
{
  struct QBusMethodProcessRecvItem* ritem = ptr;
  
  // local objects
  CapeErr err = cape_err_new ();
  
  // convert the frame content into the input message (expensive)
  QBusM qin = qbus_message_frame (ritem->frame);
  
  // call the user defined method
  qbus_methods_item__call_response (ritem->mitem, ritem->qbus, qin, NULL, err);
  
  qbus_message_del (&qin);
  cape_err_del (&err);

  qbus_methods_recv_item_del (&ritem);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods_recv__process_methods (void* ptr, number_t pos, number_t queue_size)
{
  struct QBusMethodProcessRecvItem* ritem = ptr;
  
  // local objects
  CapeErr err = cape_err_new ();
  
  // convert the frame content into udc (expensive)
  CapeUdc methods = qbus_frame_get_udc (ritem->frame);
    
  qbus_methods_item__call_methods (ritem->mitem, ritem->qbus, methods, err);
    
  cape_udc_del (&methods);
  cape_err_del (&err);
  
  qbus_methods_recv_item_del (&ritem);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods_recv__process_request (void* ptr, number_t pos, number_t queue_size)
{
  int res;
  
  struct QBusMethodProcessRecvItem* ritem = ptr;
  
  // local objects
  CapeErr err = cape_err_new ();
  
  // convert the frame content into the input message (expensive)
  QBusM qin = qbus_message_frame (ritem->frame);
  QBusM qout = qbus_message_new (NULL, NULL);
  
  res = qbus_methods_item__call_request (ritem->mitem, ritem->qbus, qin, qout, err);
  
  switch (res)
  {
    case CAPE_ERR_CONTINUE:
    {
      
      break;
    }
    default:
    {
      // override the frame content with the output message (expensive)
      {
        // TODO: check why we return the rinfo!!
        CapeUdc rinfo = qbus_frame_set_qmsg (ritem->frame, qout, err);
        cape_udc_del (&rinfo);
      }
      
      qbus_frame_set_type (ritem->frame, QBUS_FRAME_TYPE_MSG_RES, ritem->sender);
      
      // finally send the frame
      qbus_engines__send (ritem->engines, ritem->frame, ritem->conn);
    }
  }
  
  qbus_message_del (&qin);
  qbus_message_del (&qout);
  cape_err_del (&err);
  
  // this was only a reference
  ritem->mitem = NULL;
  
  qbus_methods_recv_item_del (&ritem);
}

//-----------------------------------------------------------------------------

void qbus_methods_recv_request (QBusMethods self, QBusFrame* p_frame, QBusPvdConnection conn, const CapeString sender)
{
  QBusFrame frame = *p_frame;

  {
    QBusMethodItem mitem = qbus_methods_get (self, frame->method);
    
    if (mitem)
    {
      // add to background processes
      cape_queue_add (self->queue, NULL, qbus_methods_recv__process_request, NULL, qbus_methods_recv_item_new (self->qbus, self->engines, conn, &mitem, p_frame, sender), 0);
    }
    else
    {
      CapeErr err = cape_err_new ();
      
      cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", frame->method);
      
      qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, sender);
      qbus_frame_set_err (frame, err);
      
      // finally send the frame
      qbus_engines__send (self->engines, frame, conn);
      
      cape_err_del (&err);
    }  
  }
}

//-----------------------------------------------------------------------------

void qbus_methods_recv_response (QBusMethods self, QBusFrame* p_frame, QBusPvdConnection conn, const CapeString sender)
{
  QBusFrame frame = *p_frame;
  
  if (frame->chain_key)
  {
    QBusMethodItem mitem = qbus_chain_ext (self->chain, frame->chain_key);
      
    if (mitem)
    {
      switch (qbus_methods_item_type (mitem))
      {
        case QBUS_METHOD_TYPE__FORWARD:
        {
          // handle special case for forward
          // transfer logic to qbus main class
          qbus_forward (self->qbus, *p_frame, &(mitem->chain_sender), &(mitem->chain_key));
          
          break;
        }
        case QBUS_METHOD_TYPE__REQUEST:
        {
          // this should not happen
          
          break;
        }
        case QBUS_METHOD_TYPE__RESPONSE:
        {
          // add to background processes
          cape_queue_add (self->queue, NULL, qbus_methods_recv__process_response, NULL, qbus_methods_recv_item_new (self->qbus, self->engines, conn, &mitem, p_frame, sender), 0);
          
          break;
        }
        case QBUS_METHOD_TYPE__METHODS:
        {
          // add to background processes
          cape_queue_add (self->queue, NULL, qbus_methods_recv__process_methods, NULL, qbus_methods_recv_item_new (self->qbus, self->engines, conn, &mitem, p_frame, sender), 0);
                    
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

void qbus_methods_recv_methods (QBusMethods self, QBusFrame frame, QBusPvdConnection conn, const CapeString sender)
{
  CapeUdc method_list = qbus_methods_generate_methods_list (self);
    
  qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, sender);
  qbus_frame_set_udc (frame, QBUS_FRAME_TYPE_MSG_RES, &method_list);
  
  // finally send the frame
  qbus_engines__send (self->engines, frame, conn);
}

//-----------------------------------------------------------------------------

void qbus_methods_recv_forward (QBusMethods self, QBusFrame frame, QBusPvdConnection conn, const CapeString sender)
{
  CapeString forward_chain_key = cape_str_uuid();
  CapeString forward_sender = cape_str_cp (sender);
  
  {
    QBusMethodItem mitem = qbus_methods_item_new (QBUS_METHOD_TYPE__FORWARD, NULL, NULL, NULL, NULL);
    
    // extract chain key and sender
    qbus_methods_item_continue (mitem, &(frame->chain_key), &(frame->sender), NULL);
    
    qbus_chain_add__cp (self->chain, forward_chain_key, &mitem);
  }
  
  qbus_frame_set_chainkey (frame, &forward_chain_key);
  qbus_frame_set_sender (frame, &forward_sender);
  
  // finally send the frame
  qbus_engines__send (self->engines, frame, conn);
}

//-----------------------------------------------------------------------------

struct QBusMethodProcessProcItem
{
  QBusMethodItem mitem;  // reference
  QBus qbus;             // reference
  
  QBusM msg;
  CapeString chain_key;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_methods_proc_request__process (void* ptr, number_t pos, number_t queue_size)
{
  int res;
  
  struct QBusMethodProcessProcItem* pitem = ptr;

  // local objects
  CapeErr err = cape_err_new ();
  QBusM qout = qbus_message_new (NULL, NULL);
  
  res = qbus_methods_item__call_request (pitem->mitem, pitem->qbus, pitem->msg, qout, err);
  
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
      
      // TODO
      
      /*
      // correct chain parameters
      cape_str_replace_cp (&(qout->chain_key), last_chain_key);
      cape_str_replace_cp (&(qout->sender), last_sender);
      
      // add rinfo
      cape_udc_replace_cp (&(qout->rinfo), msg->rinfo);
      
      {
        // create a method object to re-use existing functionality
        QBusMethodItem qmeth_next = qbus_methods_item_new (QBUS_METHOD_TYPE__RESPONSE, NULL, user_ptr, user_fct, NULL);
        
        // provide the last chain key to handle the return chain traversal path
        qbus_methods_item_continue (qmeth_next, &last_chain_key, &last_sender, &(msg->rinfo));
        
        // this requests ends here, now send the results back
        qbus_methods_item__call_response (qmeth_next, qbus, qout, NULL, err);
        
        qbus_methods_item_del (&qmeth_next);
      }
      */
      break;
    }
  }
  
  qbus_message_del (&qout);
  cape_err_del (&err);
  
  qbus_message_del (&(pitem->msg));
  cape_str_del (&(pitem->chain_key));
  
  CAPE_DEL(&pitem, struct QBusMethodProcessProcItem); 
}

//-----------------------------------------------------------------------------

void qbus_methods_proc_request (QBusMethods self, const CapeString method, const CapeString sender, QBusM msg, void* user_ptr, fct_qbus_onMessage user_fct)
{
  // check if we do have this method
  QBusMethodItem mitem = qbus_methods_get (self, method);
  
  if (mitem)
  {
    // create a new chain key
    CapeString next_chain_key = cape_str_uuid ();

    // create a chain entry for response path
    /*
    {
      QBusMethodItem mitem_response = qbus_methods_item_new (QBUS_METHOD_TYPE__RESPONSE, method, user_ptr, user_fct, NULL);
      
      qbus_methods_item_continue (mitem, &(msg->chain_key), &(msg->sender), &(msg->rinfo));
      
      qbus_chain_add (self->chain, next_chain_key, &mitem_response);
    }
    */
    
    // store variables and references to a temporary struct
    {
      struct QBusMethodProcessProcItem* pitem = CAPE_NEW (struct QBusMethodProcessProcItem);
      
      pitem->mitem = mitem;
      pitem->qbus = self->qbus;
      
      pitem->msg = qbus_message_data_mv (msg);
      pitem->chain_key = cape_str_mv (&next_chain_key);
      
      // add to background processes
      cape_queue_add (self->queue, NULL, qbus_methods_proc_request__process, NULL, pitem, 0);
    }
  }
  else
  {
    // TODO
    
    
  }
}

//-----------------------------------------------------------------------------

void qbus_methods_send_request (QBusMethods self, QBusPvdConnection conn, const CapeString module, const CapeString method, const CapeString sender, QBusM msg, int cont, void* user_ptr, fct_qbus_onMessage user_fct)
{
  // create a new frame
  QBusFrame frame = qbus_frame_new ();
  
  // local objects
  CapeString next_chainkey = cape_str_uuid();
  
  // add default content
  qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_REQ, next_chainkey, module, method, sender);
  
  // add message content
  msg->rinfo = qbus_frame_set_qmsg (frame, msg, NULL);
  
  {
    QBusMethodItem mitem = qbus_methods_item_new (QBUS_METHOD_TYPE__RESPONSE, method, user_ptr, user_fct, NULL);

    qbus_methods_item_continue (mitem, cont ? &(msg->chain_key): NULL, &(msg->sender), &(msg->rinfo));
    
    qbus_chain_add__mv (self->chain, &next_chainkey, &mitem);
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
  
  qbus_frame_set (frame, QBUS_FRAME_TYPE_METHODS, next_chainkey, module, NULL, sender);

  {
    QBusMethodItem mitem = qbus_methods_item_new (QBUS_METHOD_TYPE__METHODS, NULL, user_ptr, NULL, user_fct);
    
    qbus_chain_add__mv (self->chain, &next_chainkey, &mitem);
  }
    
  // finally send the frame
  qbus_engines__send (self->engines, frame, conn);

  qbus_frame_del (&frame);
}

//-----------------------------------------------------------------------------
