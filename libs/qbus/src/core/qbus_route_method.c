#include "qbus_route_method.h"
#include "qbus_connection.h"

// cape includes
#include "sys/cape_log.h"
#include "fmt/cape_json.h"

// c includes
#include <stdio.h>

//-----------------------------------------------------------------------------

struct QBusMethod_s
{
  int type;
  
  void* ptr;
  
  fct_qbus_onMessage onMsg;
  
  fct_qbus_onRemoved onRm;
  
  // for continue
  
  CapeString chain_key;
  
  CapeString chain_sender;
  
  CapeUdc rinfo;
};

//-----------------------------------------------------------------------------

QBusMethod qbus_method_new (int type, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm)
{
  QBusMethod self = CAPE_NEW (struct QBusMethod_s);
  
  self->type = type;
  
  self->ptr = ptr;
  self->onMsg = onMsg;
  self->onRm = onRm;
  
  self->chain_key = NULL;
  self->chain_sender = NULL;
  
  self->rinfo = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_method_del (QBusMethod* p_self)
{
  QBusMethod self = *p_self;
  
  if (self->onRm)
  {
    self->onRm (self->ptr);
  }
  
  if (self->rinfo)
  {
    cape_udc_del (&(self->rinfo));
  }
  
  cape_str_del (&(self->chain_key));
  cape_str_del (&(self->chain_sender));
  
  CAPE_DEL (p_self, struct QBusMethod_s);
}

//-----------------------------------------------------------------------------

void qbus_method_continue (QBusMethod self, CapeString* p_chain_key, CapeString* p_chain_sender, CapeUdc* p_rinfo)
{
  cape_str_replace_mv (&(self->chain_key), p_chain_key);
  cape_str_replace_mv (&(self->chain_sender), p_chain_sender);
  
  self->rinfo = *p_rinfo;
  *p_rinfo = NULL;
}

//-----------------------------------------------------------------------------

int qbus_method_call_request (QBusMethod self, QBus qbus, QBusFrame frame, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->onMsg)
  {
    // convert the frame content into the input message (expensive)
    QBusM qin = qbus_frame_qin (frame);
    
    // create an empty output message
    QBusM qout = qbus_message_new (NULL, NULL);
    
    // call the original callback
    res = self->onMsg (qbus, self->ptr, qin, qout, err);
    
    // override the frame content with the output message (expensive)
    qbus_frame_set_qmsg (frame, qout, err);
    
    // cleanup
    qbus_message_del (&qin);
    qbus_message_del (&qout);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_request__msg (QBusMethod self, QBus qbus, QBusM qin, QBusM qout, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->onMsg)
  {
    // call the original callback
    res = self->onMsg (qbus, self->ptr, qin, qout, err);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_response__continue_chain (QBusMethod self, QBus qbus, QBusRoute route, QBusFrame frame_original, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  QBusM qout = NULL;
  QBusM qin = NULL;
  
  // convert the frame content into the input message (expensive)
  qin = qbus_frame_qin (frame_original);
  
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
  
  switch (self->onMsg (qbus, self->ptr, qin, qout, err))
  {
    case CAPE_ERR_CONTINUE:
    {
      break;
    }
    default:
    {
      // use the correct chain key
      cape_str_replace_cp (&(qout->chain_key), self->chain_key);
      
      // send the response
      qbus_route_response (route, self->chain_sender, qout, err);
      
      break;
    }
  }
  
  // cleanup
  qbus_message_del (&qin);
  qbus_message_del (&qout);
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_response__response (QBusMethod self, QBus qbus, QBusRoute route, QBusFrame frame, CapeErr err)
{
  int res;
  
  // convert the frame content into the input message (expensive)
  QBusM qin = qbus_frame_qin (frame);
  
  // call the original callback
  res = self->onMsg (qbus, self->ptr, qin, NULL, err);
  
  // cleanup
  qbus_message_del (&qin);
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_response (QBusMethod self, QBus qbus, QBusRoute route, QBusFrame frame, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->onMsg)
  {
    if (self->chain_key)
    {
      return qbus_method_call_response__continue_chain (self, qbus, route, frame, err);
    }
    else
    {
      return qbus_method_call_response__response (self, qbus, route, frame, err);
    }
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_method_handle_request (QBusMethod self, QBus qbus, QBusRoute route, QBusConnection conn, const CapeString ident, QBusFrame* p_frame)
{
  switch (self->type)
  {
    case QBUS_METHOD_TYPE__REQUEST:
    {
      CapeErr err = cape_err_new ();
      
      switch (qbus_method_call_request (self, qbus, *p_frame, err))
      {
        case CAPE_ERR_CONTINUE:
        {
          break;
        }
        default:
        {
          qbus_frame_set_type (*p_frame, QBUS_FRAME_TYPE_MSG_RES, ident);
          
          // finally send the response frame
          qbus_connection_send (conn, p_frame);
        }
      }
      
      cape_err_del (&err);
      
      break;
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_method_handle_response (QBusMethod self, QBus qbus, QBusRoute route, QBusFrame* p_frame)
{
  switch (self->type)
  {
    case QBUS_METHOD_TYPE__REQUEST:
    {
      // this should not happen
      
      break;
    }
    case QBUS_METHOD_TYPE__RESPONSE:
    {
      CapeErr err = cape_err_new ();
      
      qbus_method_call_response (self, qbus, route, *p_frame, err);
      
      cape_err_del (&err);
      
      break;
    }
    case QBUS_METHOD_TYPE__FORWARD:
    {
      qbus_route_on_msg_forward (route, p_frame, &(self->ptr));
      
      break;
    }
    case QBUS_METHOD_TYPE__METHODS:
    {
      qbus_route_on_msg_methods (route, p_frame, &(self->ptr));
      
      break;
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_method_local (QBusMethod self, QBus qbus, QBusRoute route, QBusM msg, QBusM qout, const CapeString method, CapeErr err)
{
  int res;
  
  switch (self->type)
  {
    case QBUS_METHOD_TYPE__REQUEST:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call method '%s'", method);
      
      res = qbus_method_call_request__msg (self, qbus, msg, qout, err);
      break;
    }
    default:
    {
      res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", method);
      break;
    }
  }
  
  return res;
}

//-----------------------------------------------------------------------------
