#include "qbus_route.h"
#include "qbus_connection.h"
#include "qbus_route_items.h"
#include "qbus_route_method.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_mutex.h"
#include "sys/cape_log.h"
#include "stc/cape_map.h"
#include "fmt/cape_json.h"
#include "sys/cape_queue.h"

// c includes
#include <stdio.h>

//-----------------------------------------------------------------------------

typedef struct
{
  
  CapeString chain_key;
  
  CapeString sender;
  
} QBusForwardData;

//-----------------------------------------------------------------------------

typedef struct
{
  void* ptr;
  
  fct_qbus_on_methods on_methods;
  
} QBusMethodsData;

//-----------------------------------------------------------------------------

struct QBusOnChangeCallback_s
{
  fct_qbus_on_route_change fct;
  
  void* ptr;
  
}; typedef struct QBusOnChangeCallback_s* QBusOnChangeCallback;

//-----------------------------------------------------------------------------

static void __STDCALL qbus_route_callbacks_on_del (void* ptr)
{
  QBusOnChangeCallback cc = ptr;
  
  CAPE_DEL(&cc, struct QBusOnChangeCallback_s);
}

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  QBus qbus;   // reference 
  
  CapeString name;
  CapeString uuid;
  
  CapeMap methods;
  
  CapeMap chains;
  
  CapeMutex chain_mutex;
  
  QBusRouteItems route_items;  
  
  // for on change
  
  CapeList on_changes_callbacks;
  
  CapeMutex on_changes_mutex;
  
  CapeQueue queue;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_methods_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethod qmeth = val; qbus_method_del (&qmeth);
  }
}

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new (QBus qbus, const CapeString name)
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);
  
  self->qbus = qbus;
  
  self->name = cape_str_cp (name);
  self->uuid = cape_str_uuid ();
  
  self->methods = cape_map_new (NULL, qbus_route_methods_del, NULL);
  
  self->chain_mutex = cape_mutex_new ();
  self->chains = cape_map_new (NULL, qbus_route_methods_del, NULL);
  
  self->route_items = qbus_route_items_new ();
  
  self->on_changes_callbacks = cape_list_new (qbus_route_callbacks_on_del);
  self->on_changes_mutex = cape_mutex_new ();
  
  self->queue = cape_queue_new (300000);   // maximum of 5 minutes
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  QBusRoute self = *p_self;
  
  cape_queue_del (&(self->queue));
  
  cape_str_del (&(self->name));
  cape_str_del (&(self->uuid));

  cape_map_del (&(self->methods));
  
  cape_mutex_del (&(self->chain_mutex));
  cape_map_del (&(self->chains));
  
  qbus_route_items_del (&(self->route_items));
  
  cape_list_del (&(self->on_changes_callbacks));
  cape_mutex_del (&(self->on_changes_mutex));
  
  CAPE_DEL (p_self, struct QBusRoute_s);
}

//-----------------------------------------------------------------------------

int qbus_route_init (QBusRoute self, number_t threads, CapeErr err)
{
  cape_log_fmt (CAPE_LL_INFO, "QBUS", "route init", "start with %i worker threads", threads);
  
  // TODO: to avoid that qbus get stuck try 1 thread here
  //return cape_queue_start (self->queue, 1, err);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_route_conn_reg (QBusRoute self, QBusConnection conn)
{
  // log
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "establish a new connection to QBUS as: %s  = %s", self->name, self->uuid);
  
  // send first frame
  {
    QBusFrame frame = qbus_frame_new ();
    
    // CH01: replace self->name with self->uuid and add name as module
    qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_REQ, NULL, self->name, NULL, self->uuid);
    
    // finally send the frame
    qbus_connection_send (conn, &frame);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_send_updates (QBusRoute self, QBusConnection conn_origin)
{
  CapeList list_of_all_connections = qbus_route_items_conns (self->route_items, conn_origin);
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (list_of_all_connections, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      CapeUdc route_nodes;

      QBusConnection conn = cape_list_node_data (cursor->node);
      
      QBusFrame frame = qbus_frame_new ();
      
      qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_UPD, NULL, NULL, NULL, self->name);
      
      route_nodes = qbus_route_items_nodes (self->route_items);
      
      {
        CapeString h = cape_json_to_s (route_nodes);
        
        // log
        //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "route update", "send route update: %s -> %s", h, qbus_connection_get (conn));
        
        cape_str_del(&h);
      }
      
      if (route_nodes)
      {
        // set the payload frame
        qbus_frame_set_udc (frame, QBUS_MTYPE_JSON, &route_nodes);
      }
      
      // finally send the frame
      qbus_connection_send (conn, &frame);
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_list_del (&list_of_all_connections);
}

//-----------------------------------------------------------------------------

void qbus_route_conn_rm (QBusRoute self, QBusConnection conn)
{
  const CapeString module_name = qbus_connection__name (conn);
  const CapeString module_uuid = qbus_connection__uuid (conn);

  // log
  cape_log_msg (CAPE_LL_TRACE, "QBUS", "conn reg", "connection dropped");
  
  if (module_name)
  {
    qbus_route_items_rm (self->route_items, module_name, module_uuid);
  }
  else
  {
    
  }
  
  qbus_route_send_updates (self, conn);  
  
  {
    CapeUdc modules = qbus_route_items_nodes (self->route_items);

    qbus_route_run_on_change (self, &modules);
  }
}

//-----------------------------------------------------------------------------

QBusConnection const qbus_route_module_find (QBusRoute self, const char* module_origin)
{
  return qbus_route_items_get (self->route_items, module_origin, NULL);
}

//-----------------------------------------------------------------------------

void qbus_route_on_route_request (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  CapeUdc route_nodes;

  QBusFrame frame = *p_frame;
  
  const CapeString module = qbus_frame_get_module (frame);
  const CapeString sender = qbus_frame_get_sender (frame);

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "request info: module = %s, sender = %s", module, sender);

  if (cape_str_empty (module))
  {
    // old version
    qbus_frame_set_type (frame, QBUS_FRAME_TYPE_ROUTE_RES, self->name);
  }
  else
  {
    // reset the type and the sender
    qbus_frame_set_type (frame, QBUS_FRAME_TYPE_ROUTE_RES, self->uuid);

    // reset the module
    // -> module is used as name for the module
    qbus_frame_set_module__cp (frame, self->name);
  }
  
  route_nodes = qbus_route_items_nodes (self->route_items);

  if (route_nodes)
  {
    // set the payload frame
    qbus_frame_set_udc (frame, QBUS_MTYPE_JSON, &route_nodes);
  }
  
  // finally send the frame
  qbus_connection_send (conn, p_frame);
}

//-----------------------------------------------------------------------------

void qbus_route_on_route_response (QBusRoute self, QBusConnection conn, QBusFrame frame)
{
  CapeUdc route_nodes = qbus_frame_get_udc (frame);
  
  const CapeString module = qbus_frame_get_module (frame);
  const CapeString sender = qbus_frame_get_sender (frame);
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "retrieve node info: module = %s, sender = %s", module, sender);

  if (cape_str_equal (self->name, module))
  {
    // support old version
    // -> old version has a simple relay mechanism
    qbus_route_items_add (self->route_items, sender, NULL, conn, &route_nodes);
  }
  else
  {
    qbus_route_items_add (self->route_items, module, sender, conn, &route_nodes);
  }
  
  // tell the others the new nodes
  qbus_route_send_updates (self, conn);  

  {
    CapeUdc modules = qbus_route_items_nodes (self->route_items);
    
    qbus_route_run_on_change (self, &modules);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_on_route_update (QBusRoute self, QBusConnection conn, QBusFrame frame)
{
  const CapeString module_name = qbus_connection__name (conn);
  const CapeString module_uuid = qbus_connection__uuid (conn);

  if (module_name)
  {
    CapeUdc route_nodes = qbus_frame_get_udc (frame);

    if (route_nodes)
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "update route items %s = %s", module_name, module_uuid);
      
      qbus_route_items_update (self->route_items, conn, &route_nodes);
    }

    {
      CapeUdc modules = qbus_route_items_nodes (self->route_items);
      
      qbus_route_run_on_change (self, &modules);
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_foward (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;

  CapeString chain_key;

  QBusForwardData* qbus_fd = CAPE_NEW (QBusForwardData);
  
  // create a copy of the chain key
  qbus_fd->chain_key = cape_str_cp (qbus_frame_get_chainkey  (frame));
  qbus_fd->sender = cape_str_cp (qbus_frame_get_sender  (frame));

  // create a new chain key
  chain_key = cape_str_uuid();
  
  cape_mutex_lock (self->chain_mutex);
  
  {
    QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__FORWARD, qbus_fd, NULL, NULL);
    
    // transfer ownership of chain_key to the map
    cape_map_insert (self->chains, (void*)chain_key, (void*)qmeth);
  }
  
  cape_mutex_unlock (self->chain_mutex);
  
  // chain key
  {
    CapeString h = cape_str_cp (chain_key);
    
    qbus_frame_set_chainkey (frame, &h);
  }
    
  // sender
  {
    CapeString sender = cape_str_cp (self->name);
    
    qbus_frame_set_sender (frame, &sender);
  }
    
  // forward the frame
  qbus_connection_send (conn, p_frame);
}

//-----------------------------------------------------------------------------

QBusMethod qbus_route__find_method (QBusRoute self, const char* method_origin, CapeErr err)
{
  QBusMethod ret = NULL;
  CapeMapNode n;
  
  // local objects
  CapeString method = cape_str_cp (method_origin);
  
  // convert into lower case
  cape_str_to_lower (method);
  
  // try to find the method
  n = cape_map_find (self->methods, method);
  if (n == NULL)
  {
    cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", method);
    goto exit_and_cleanup;
  }
  
  // get the methods object
  ret = cape_map_node_value (n);
  
exit_and_cleanup:
  
  cape_str_del (&method);
  return ret;
}

//-----------------------------------------------------------------------------

struct QbusRouteResponseWorkerCtx_s
{
  QBusRoute route;        // reference
  QBusConnection conn;    // reference
  QBusFrame frame;        // owned
  
}; typedef struct QbusRouteResponseWorkerCtx_s* QbusRouteResponseWorkerCtx;

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_on_msg_method__worker (void* ptr, number_t pos, number_t queue_size)
{
  QbusRouteResponseWorkerCtx ctx = ptr;

  // local objects
  CapeErr err = cape_err_new ();
  
  QBusMethod qmeth = qbus_route__find_method (ctx->route, qbus_frame_get_method (ctx->frame), err);
  if (qmeth)
  {
    qbus_method_handle_request (qmeth, ctx->route->qbus, ctx->route, ctx->conn, ctx->route->name, &(ctx->frame));
  }
  else
  {
    qbus_frame_set_type (ctx->frame, QBUS_FRAME_TYPE_MSG_RES, ctx->route->name);
    qbus_frame_set_err (ctx->frame, err);
    
    // finally send the frame
    qbus_connection_send (ctx->conn, &(ctx->frame));
  }
  
  cape_err_del (&err);

  // cleanup
  qbus_frame_del (&(ctx->frame));
  
  CAPE_DEL (&ctx, struct QbusRouteResponseWorkerCtx_s);
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_method (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QbusRouteResponseWorkerCtx ctx = CAPE_NEW (struct QbusRouteResponseWorkerCtx_s);
  
  ctx->route = self;
  ctx->conn = conn;
  
  // transfer frame
  ctx->frame = *p_frame;
  *p_frame = NULL;
  
  qbus_route_on_msg_method__worker (ctx, 0, 0);
  
  //cape_queue_add (self->queue, NULL, qbus_route_on_msg_method__worker, NULL, ctx, 0);
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_request (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  const CapeString module = qbus_frame_get_module (frame);
  
  // check if the message was sent to us
  if (cape_str_compare (module, self->name))
  {
    qbus_route_on_msg_method (self, conn, p_frame);
  }
  else  // the message was not send to us -> forward it 
  {
    // try to find a connection which might reach the destination module
    QBusConnection conn_forward = qbus_route_items_get (self->route_items, module, NULL);
    if (conn_forward)
    {
      qbus_route_on_msg_foward (self, conn_forward, p_frame);
    }
    else
    {
      CapeErr err = cape_err_new ();
      
      cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "no route to %s", module);
      
      qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
      
      qbus_frame_set_err (frame, err);
      
      cape_err_del (&err);
      
      // finally send the frame
      qbus_connection_send (conn, p_frame);
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_forward (QBusRoute self, QBusFrame* p_frame, void* ptr)
{
  QBusFrame frame = *p_frame;
  
  QBusForwardData* qbus_fd = ptr;
  
  // try to find a connection which might reach the destination module
  QBusConnection conn_forward = qbus_route_items_get (self->route_items, qbus_fd->sender, NULL);
  if (conn_forward)
  {
    qbus_frame_set_chainkey (frame, &(qbus_fd->chain_key));
    qbus_frame_set_sender (frame, &(qbus_fd->sender));
    
    // forward the frame
    qbus_connection_send (conn_forward, p_frame);
  }
  else
  {
    // log
    cape_log_msg (CAPE_LL_ERROR, "QBUS", "msg forward", "forward message can't be returned");
  }
  
  cape_str_del (&(qbus_fd->chain_key));
  cape_str_del (&(qbus_fd->sender));
  
  CAPE_DEL(&qbus_fd, QBusForwardData);
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_methods (QBusRoute self, QBusFrame* p_frame, void* ptr)
{
  QBusFrame frame = *p_frame;

  QBusMethodsData* qbus_methods = ptr;
  
  if (qbus_methods->on_methods)
  {
    CapeErr err = cape_err_new ();
    
    CapeUdc methods = qbus_frame_get_udc (frame);
    
    qbus_methods->on_methods (self->qbus, qbus_methods->ptr, methods, err);

    cape_udc_del (&methods);
    
    cape_err_del (&err);
  }
  
  CAPE_DEL(&qbus_methods, QBusMethodsData);
}

//-----------------------------------------------------------------------------

void qbus_route__response__method (QBusRoute self, QBusMethod qmeth, QBusFrame* p_frame)
{
  switch (qbus_method_type (qmeth))
  {
    case QBUS_METHOD_TYPE__REQUEST:
    {
      // this should not happen
      
      break;
    }
    case QBUS_METHOD_TYPE__RESPONSE:
    {
      CapeErr err = cape_err_new ();
      
      // convert the frame content into the input message (expensive)
      QBusM qin = qbus_frame_qin (*p_frame);

      qbus_method_call_response (qmeth, self->qbus, self, qin, NULL, err);
      
      qbus_message_del (&qin);
      cape_err_del (&err);

      break;
    }
    case QBUS_METHOD_TYPE__FORWARD:
    {
      qbus_route_on_msg_forward (self, p_frame, qbus_method_ptr (qmeth));
      
      break;
    }
    case QBUS_METHOD_TYPE__METHODS:
    {
      qbus_route_on_msg_methods (self, p_frame, qbus_method_ptr (qmeth));
      
      break;
    }
  }
}

//-----------------------------------------------------------------------------

QBusMethod qbus_route__find_chain (QBusRoute self, const CapeString chain_key)
{
  QBusMethod ret = NULL;
  CapeMapNode n;
  
  cape_mutex_lock (self->chain_mutex);
  
  n = cape_map_find (self->chains, (void*)chain_key);
  
  if (n)
  {
    cape_map_extract (self->chains, n);
  }
  
  cape_mutex_unlock (self->chain_mutex);

  if (n)
  {
    // removes the value from the map-node
    ret = cape_map_node_mv (n);

    // free memory and the node key
    cape_map_del_node (self->chains, &n);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_on_msg_response__worker (void* ptr, number_t pos, number_t queue_size)
{
  QbusRouteResponseWorkerCtx ctx = ptr;
  
  const CapeString chain_key = qbus_frame_get_chainkey (ctx->frame);
  
  if (chain_key)
  {
    QBusMethod qmeth = qbus_route__find_chain (ctx->route, chain_key);
    
    if (qmeth)
    {
      qbus_route__response__method (ctx->route, qmeth, &(ctx->frame));

      qbus_method_del (&qmeth);
    }
    else
    {
      cape_log_fmt (CAPE_LL_ERROR, "QBUS", "msg response", "can't find chainkey for = %s", chain_key);
    }
  }
  else
  {
    cape_log_msg (CAPE_LL_WARN, "QBUS", "msg response", "no chainkey");
  }

  // cleanup context
  qbus_frame_del (&(ctx->frame));
  
  CAPE_DEL (&ctx, struct QbusRouteResponseWorkerCtx_s);
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_response (QBusRoute self, QBusFrame* p_frame)
{
  QbusRouteResponseWorkerCtx ctx = CAPE_NEW (struct QbusRouteResponseWorkerCtx_s);
  
  ctx->route = self;
  
  // transfer frame
  ctx->frame = *p_frame;
  *p_frame = NULL;

  qbus_route_on_msg_response__worker (ctx, 0, 0);
  
  //cape_queue_add (self->queue, NULL, qbus_route_on_msg_response__worker, NULL, ctx, 0);
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route__generate_modules_list (QBusRoute self)
{
  // encode methods into list
  CapeUdc method_list = cape_udc_new (CAPE_UDC_LIST, NULL);

  // iterate through all methods
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->methods, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      //QBusMethod qmeth = cape_map_node_value (cursor->node);
     
      //CapeUdc method_node = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      //cape_udc_add_s_cp (method_node, "name", cape_map_node_key (cursor->node));
      
      //cape_udc_add (method_list, &method_node);
      
      cape_udc_add_s_cp (method_list, NULL, cape_map_node_key (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  return method_list;
}

//-----------------------------------------------------------------------------

void qbus_route_on_route_methods_request (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  const CapeString module = qbus_frame_get_module (frame);
  
  // check if the message was sent to us
  if (cape_str_compare (module, self->name))
  {
    // encode methods into list
    CapeUdc method_list = qbus_route__generate_modules_list (self);

    /*
    {
      CapeString h = cape_json_to_s (method_list);
      
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "methods req", "send methods: %s", h);
      
      cape_str_del(&h);
    }
    */

    qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
    qbus_frame_set_udc (frame, QBUS_FRAME_TYPE_MSG_RES, &method_list);
    
    // send the frame back
    qbus_connection_send (conn, p_frame);
  }
  else  // the message was not send to us -> forward it 
  {
    // try to find a connection which might reach the destination module
    QBusConnection conn_forward = qbus_route_items_get (self->route_items, module, NULL);
    if (conn_forward)
    {
      qbus_route_on_msg_foward (self, conn_forward, p_frame);
    }
    else
    {
      CapeErr err = cape_err_new ();
      
      cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "no route to %s", module);
      
      qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
      
      qbus_frame_set_err (frame, err);
      
      cape_err_del (&err);
      
      // finally send the frame
      qbus_connection_send (conn, p_frame);
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route__on_observable_request (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  const CapeString module = qbus_frame_get_module (frame);
  
  // check if the message was sent to us
  if (cape_str_compare (module, self->name))
  {

  }
  else
  {
    
  }
}

//-----------------------------------------------------------------------------

void qbus_route__on_observable_response (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;

  const CapeString chain_key = qbus_frame_get_chainkey (frame);

  
}

//-----------------------------------------------------------------------------

void qbus_route_conn_onFrame (QBusRoute self, QBusConnection connection, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  switch (qbus_frame_get_type (frame))
  {
    case QBUS_FRAME_TYPE_ROUTE_REQ:
    {
      qbus_route_on_route_request (self, connection, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_ROUTE_RES:
    {
      qbus_route_on_route_response (self, connection, frame);
      break;
    }
    case QBUS_FRAME_TYPE_ROUTE_UPD:
    {
      qbus_route_on_route_update (self, connection, frame);
      break;
    }
    case QBUS_FRAME_TYPE_MSG_REQ:
    {
      qbus_route_on_msg_request (self, connection, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_MSG_RES:
    {
      qbus_route_on_msg_response (self, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_METHODS:
    {
      qbus_route_on_route_methods_request (self, connection, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_OBSVBL_REQ:
    {
      qbus_route__on_observable_request (self, connection, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_OBSVBL_RES:
    {
      qbus_route__on_observable_response (self, connection, p_frame);
      break;
    }
  }
  
  qbus_frame_del (p_frame);    
}

//-----------------------------------------------------------------------------

void qbus_route_meth_reg (QBusRoute self, const char* method_origin, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm)
{
  QBusMethod qmeth;
  CapeString method = cape_str_cp (method_origin);
  
  cape_str_to_lower (method);

  qmeth = qbus_method_new (QBUS_METHOD_TYPE__REQUEST, ptr, onMsg, onRm);
  
  cape_map_insert (self->methods, (void*)method, (void*)qmeth);
}

//-----------------------------------------------------------------------------

void qbus_route_no_route (QBusRoute self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  // log
  cape_log_fmt (CAPE_LL_WARN, "QBUS", "msg forward", "no route to module %s", module);
  
  if (onMsg)
  {
    if (msg->err)
    {
      cape_err_del (&(msg->err));
    }
    
    // create a new error object
    msg->err = cape_err_new ();
    
    // set the error
    cape_err_set (msg->err, CAPE_ERR_NOT_FOUND, "no route to module");
    
    {
      // create a temporary error object
      CapeErr err = cape_err_new ();
      
      int res = onMsg (self->qbus, ptr, msg, NULL, err);
      if (res)
      {
        // TODO: handle error
        
      }
      
      cape_err_del (&err);
    }      
  }  
}

//-----------------------------------------------------------------------------

void qbus_route__add_to_chain (QBusRoute self, void* ptr, fct_qbus_onMessage onMsg, CapeString* p_last_chainkey, CapeString* p_next_chainkey, CapeString* p_last_sender, CapeUdc* p_rinfo)
{
  QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__RESPONSE, ptr, onMsg, NULL);
  
  if (p_last_chainkey)
  {
    //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "add chainkey '%s' for continue", msg->chain_key);
    
    qbus_method_continue (qmeth, p_last_chainkey, p_last_sender, p_rinfo);
  }
  
  cape_mutex_lock (self->chain_mutex);
  
  cape_map_insert (self->chains, (void*)cape_str_mv (p_next_chainkey), (void*)qmeth);
  
  cape_mutex_unlock (self->chain_mutex);
}

//-----------------------------------------------------------------------------

void qbus_route_conn_request (QBusRoute self, QBusConnection const conn, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg, int cont)
{
  // create a new frame
  QBusFrame frame = qbus_frame_new ();
  
  // local objects
  CapeString next_chainkey = cape_str_uuid();
  
  // add default content
  qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_REQ, next_chainkey, module, method, self->name);
  
  // add message content
  msg->rinfo = qbus_frame_set_qmsg (frame, msg, NULL);
  
  // register this request as response in the chain storage
  qbus_route__add_to_chain (self, ptr, onMsg, cont ? &(msg->chain_key): NULL, &next_chainkey, &(msg->sender), &(msg->rinfo));

  // finally send the frame
  qbus_connection_send (conn, &frame);
}

//-----------------------------------------------------------------------------

void qbus_route_request__local_request (QBusRoute self, const char* method_origin, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  int res;
  
  QBusMethod qmeth;
  
  // local objects
  CapeErr err = cape_err_new ();
  QBusM qout = qbus_message_new (NULL, NULL);
  
  CapeString last_chain_key = cape_str_mv (&(msg->chain_key));
  CapeString last_sender = cape_str_mv (&(msg->sender));
  
  CapeString next_chain_key = cape_str_uuid ();
  
  // set next chain key
  msg->chain_key = cape_str_cp (next_chain_key);
  msg->sender = cape_str_cp (self->name);
  
  // set default message type
  qout->mtype = QBUS_MTYPE_JSON;
  
  // try to find the method stored in route
  qmeth = qbus_route__find_method (self, method_origin, err);

  if (qmeth == NULL)
  {
    goto exit_and_cleanup;
  }
    
  {
    CapeString last_chain_key_copy = cape_str_cp (last_chain_key);
    CapeString last_sender_copy = cape_str_cp (last_sender);
    
    CapeUdc rinfo_copy = cape_udc_cp (msg->rinfo);
    
    qbus_route__add_to_chain (self, ptr, onMsg, &last_chain_key_copy, &next_chain_key, &last_sender_copy, &rinfo_copy);
    
    cape_udc_del (&rinfo_copy);
    
    cape_str_del (&last_sender_copy);
    cape_str_del (&last_chain_key_copy);
  }

  
  
  res = qbus_method_local (qmeth, self->qbus, self, msg, qout, method_origin, err);
  
  
  
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
      cape_udc_replace_cp (&(qout->rinfo), msg->rinfo);
      
      {
        // create a method object to re-use existing functionality
        QBusMethod qmeth_next = qbus_method_new (QBUS_METHOD_TYPE__RESPONSE, ptr, onMsg, NULL);

        // provide the last chain key to handle the return chain traversal path
        qbus_method_continue (qmeth_next, &last_chain_key, &last_sender, &(msg->rinfo));
        
        // this requests ends here, now send the results back
        qbus_method_call_response (qmeth_next, self->qbus, self, qout, NULL, err);
        
        qbus_method_del (&qmeth_next);
      }

      break;
    }
  }
  
exit_and_cleanup:
  
  cape_str_del (&next_chain_key);
  
  cape_str_replace_mv (&(msg->chain_key), &last_chain_key);
  cape_str_replace_mv (&(msg->sender), &last_sender);
  
  qbus_message_del (&qout);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

struct QbusRouteWorkerCtx_s
{
  QBusRoute route;   // reference
  QBusM qin;
  
  CapeString module;
  CapeString method;

  void* ptr;
  fct_qbus_onMessage onMsg;
  
  QBusConnection conn;
  int cont;
  
}; typedef struct QbusRouteWorkerCtx_s* QbusRouteWorkerCtx;

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_request__local_request__worker (void* ptr, number_t pos, number_t queue_size)
{
  QbusRouteWorkerCtx ctx = ptr;
  
  qbus_route_request__local_request (ctx->route, ctx->method, ctx->qin, ctx->ptr, ctx->onMsg);
    
  cape_str_del (&(ctx->module));
  cape_str_del (&(ctx->method));

  qbus_message_del (&(ctx->qin));

  CAPE_DEL (&ctx, struct QbusRouteWorkerCtx_s);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_request__remote_request__worker (void* ptr, number_t pos, number_t queue_size)
{
  QbusRouteWorkerCtx ctx = ptr;

  qbus_route_conn_request (ctx->route, ctx->conn, ctx->module, ctx->method, ctx->qin, ctx->ptr, ctx->onMsg, ctx->cont);
  
  cape_str_del (&(ctx->module));
  cape_str_del (&(ctx->method));

  qbus_message_del (&(ctx->qin));
  
  CAPE_DEL (&ctx, struct QbusRouteWorkerCtx_s);
}

//-----------------------------------------------------------------------------

int qbus_route_request (QBusRoute self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg, int cont, CapeErr err)
{
  if (cape_str_compare (module, self->name))
  {
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "execute local request on '%s'", module);
    
    QbusRouteWorkerCtx ctx = CAPE_NEW (struct QbusRouteWorkerCtx_s);
    
    ctx->route = self;
    ctx->qin = qbus_message_data_mv (msg);
    
    ctx->module = NULL;
    ctx->method = cape_str_cp (method);
    
    ctx->ptr = ptr;
    ctx->onMsg = onMsg;
    
    ctx->conn = NULL;
    ctx->cont = FALSE;
    
    qbus_route_request__local_request__worker (ctx, 0, 0);
    
    //cape_queue_add (self->queue, NULL, qbus_route_request__local_request__worker, NULL, ctx, 0);
    
    return CAPE_ERR_CONTINUE;
  }
  else
  {
    QBusConnection const conn = qbus_route_module_find (self, module);
    
    if (conn)
    {
      QbusRouteWorkerCtx ctx = CAPE_NEW (struct QbusRouteWorkerCtx_s);
      
      ctx->route = self;
      ctx->qin = qbus_message_data_mv (msg);

      ctx->module = cape_str_cp (module);
      ctx->method = cape_str_cp (method);
      
      ctx->ptr = ptr;
      ctx->onMsg = onMsg;
      
      ctx->conn = conn;
      ctx->cont = cont;

      qbus_route_request__remote_request__worker (ctx, 0, 0);
      
      //cape_queue_add (self->queue, NULL, qbus_route_request__remote_request__worker, NULL, ctx, 0);

      //qbus_route_conn_request (self, conn, module, method, msg, ptr, onMsg, cont);
      
      return CAPE_ERR_CONTINUE;
    }
    else
    {
      qbus_route_no_route (self, module, method, msg, ptr, onMsg);

      return cape_err_set (err, CAPE_ERR_NOT_FOUND, "no route to module");
    }
  }  
}

//-----------------------------------------------------------------------------

void qbus_route_response__local (QBusRoute self, QBusM msg)
{
  if (msg->chain_key)
  {
    QBusMethod qmeth = qbus_route__find_chain (self, msg->chain_key);
    
    if (qmeth)
    {
      switch (qbus_method_type (qmeth))
      {
        case QBUS_METHOD_TYPE__RESPONSE:
        {
          CapeErr err = cape_err_new ();
          
          qbus_method_call_response (qmeth, self->qbus, self, msg, NULL, err);
          
          cape_err_del (&err);
          
          break;
        }
      }
      
      qbus_method_del (&qmeth);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "QBUS", "response", "chain key was not found in local response");
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_response (QBusRoute self, const char* module, QBusM msg, CapeErr err)
{
  if (module)
  {
    if (cape_str_compare (module, self->name))
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "response", "execute local response on '%s'", module);
      
      qbus_route_response__local (self, msg);

      return;
    }
    else
    {
      QBusConnection const conn = qbus_route_module_find (self, module);
      if (conn)
      {
        // create a new frame
        QBusFrame frame = qbus_frame_new ();
        
        // add default content
        qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_RES, msg->chain_key, module, NULL, self->name);
        
        // add message content
        qbus_frame_set_qmsg (frame, msg, err);
        
        // finally send the frame
        qbus_connection_send (conn, &frame);
        
        return;
      }
    }
  }
  
  cape_log_fmt (CAPE_LL_ERROR, "QBUS", "route response", "no route for response '%s'", module);
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route_modules (QBusRoute self)
{
  return qbus_route_items_nodes (self->route_items);
}

//-----------------------------------------------------------------------------

void qbus_route_log_msg (QBusRoute self, const CapeString remote, const CapeString message)
{
  qbus_log_msg (self->qbus, remote, message);
}

//-----------------------------------------------------------------------------

void* qbus_route_add_on_change (QBusRoute self, void* ptr, fct_qbus_on_route_change on_change)
{
  CapeListNode n = NULL;
  
  cape_mutex_lock (self->on_changes_mutex);

  {
    QBusOnChangeCallback cc = CAPE_NEW (struct QBusOnChangeCallback_s);
    
    cc->fct = on_change;
    cc->ptr = ptr;
    
    n = cape_list_push_back (self->on_changes_callbacks, cc);
  }
  
  cape_mutex_unlock (self->on_changes_mutex);
  
  return n;
}

//-----------------------------------------------------------------------------

void qbus_route_run_on_change (QBusRoute self, CapeUdc* p_modules)
{
  cape_mutex_lock (self->on_changes_mutex);
  
  {
    CapeListCursor cursor; cape_list_cursor_init (self->on_changes_callbacks, &cursor, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (&cursor))
    {
      QBusOnChangeCallback cc = cape_list_node_data (cursor.node);
      
      if (cc->fct)
      {
        cc->fct (self->qbus, cc->ptr, *p_modules);
      }
    }    
  }
  
  cape_mutex_unlock (self->on_changes_mutex);
  
  cape_udc_del (p_modules);
}

//-----------------------------------------------------------------------------

void qbus_route_rm_on_change (QBusRoute self, void* obj)
{
  CapeListNode ret = obj;
  
  cape_mutex_lock (self->on_changes_mutex);
  
  cape_list_node_erase (self->on_changes_callbacks, ret);
  
  cape_mutex_unlock (self->on_changes_mutex);
}

//-----------------------------------------------------------------------------

void qbus_route_methods (QBusRoute self, const char* module, void* ptr, fct_qbus_on_methods on_methods)
{
  QBusConnection const conn = qbus_route_module_find (self, module);
  
  if (conn)
  {
    // create a new frame
    QBusFrame frame = qbus_frame_new ();

    // create a new chain key
    CapeString chain_key = cape_str_uuid();

    QBusMethodsData* qbus_methods = CAPE_NEW (QBusMethodsData);
    
    qbus_methods->ptr = ptr;
    qbus_methods->on_methods = on_methods;
    
    cape_mutex_lock (self->chain_mutex);
    
    {
      QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__METHODS, qbus_methods, NULL, NULL);
      
      // transfer ownership of chain_key to the map
      cape_map_insert (self->chains, (void*)chain_key, (void*)qmeth);
    }
    
    cape_mutex_unlock (self->chain_mutex);

    qbus_frame_set (frame, QBUS_FRAME_TYPE_METHODS, chain_key, module, NULL, self->name);
        
    // send the frame
    qbus_connection_send (conn, &frame);
  }
  else
  {
    if (on_methods)
    {
      CapeErr err = cape_err_new ();

      // set the error
      cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "no route to module: %s", module);
      
      // callback
      on_methods (self->qbus, ptr, NULL, err);
      
      cape_err_del (&err);
    }
  }
}

//-----------------------------------------------------------------------------
