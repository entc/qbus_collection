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

number_t qbus_method_type (QBusMethod self)
{
  return self->type;
}

//-----------------------------------------------------------------------------

void* qbus_method_ptr (QBusMethod self)
{
  return self->ptr;
}

//-----------------------------------------------------------------------------

void qbus_method_continue (QBusMethod self, CapeString* p_chain_key, CapeString* p_chain_sender, CapeUdc* p_rinfo)
{
  cape_str_replace_mv (&(self->chain_key), p_chain_key);
  cape_str_replace_mv (&(self->chain_sender), p_chain_sender);
  
  self->rinfo = cape_udc_mv (p_rinfo);
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

int qbus_method_call_response__continue_chain (QBusMethod self, QBus qbus, QBusRoute route, QBusM qin, CapeErr err)
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
      
      // use the correct sender
      cape_str_replace_cp (&(qout->sender), self->chain_sender);
      
      // send the response
      qbus_route_response (route, self->chain_sender, qout, err);
      
      break;
    }
  }
  
  // cleanup
  qbus_message_del (&qout);
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_response (QBusMethod self, QBus qbus, QBusRoute route, QBusM qin, QBusM qout, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->onMsg)
  {
    if (self->chain_key)
    {
      res = qbus_method_call_response__continue_chain (self, qbus, route, qin, err);
    }
    else
    {
      // call the original callback
      res = self->onMsg (qbus, self->ptr, qin, qout, err);
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
      
      if (self->onMsg)
      {
        // call the original callback
        res = self->onMsg (qbus, self->ptr, msg, qout, err);
      }

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
