#include "qbus_engines_pvd.h"
#include "qbus_pvd.h"
#include "qbus_frame.h"
#include "qbus_obsvbl.h"
#include "qbus_methods.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_dl.h>
#include <sys/cape_file.h>
#include <fmt/cape_json.h>
#include <stc/cape_list.h>

typedef struct
{
  fct_qbus_pvd_ctx_new         ctx_new;
  
  fct_qbus_pvd_ctx_del         ctx_del;
  
  fct_qbus_pvd_ctx_reg         ctx_reg;
  
  fct_qbus_pvd_ctx_send        ctx_send;
  
} QBusPvd2;

//-----------------------------------------------------------------------------

struct QBusEnginesPvd_s
{
  CapeDl hlib;           // handle for the shared library
  QBusPvd2 pvd2;         // function pointers
  QBusPvdCtx ctx;        // library context
  
  QBusRoute route;       // reference
  QBusObsvbl obsvbl;     // reference
  QBusMethods methods;   // reference
};

//-----------------------------------------------------------------------------

QBusEnginesPvd qbus_engines_pvd_new (QBusRoute route, QBusObsvbl obsvbl, QBusMethods methods)
{
  QBusEnginesPvd self = CAPE_NEW (struct QBusEnginesPvd_s);

  self->route = route;
  self->obsvbl = obsvbl;
  self->methods = methods;
  
  self->hlib = cape_dl_new ();
  self->ctx = NULL;
  
  memset (&(self->pvd2), 0, sizeof(QBusPvd2));
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd_del (QBusEnginesPvd* p_self)
{
  if (*p_self)
  {
    QBusEnginesPvd self = *p_self;
    
    cape_log_msg (CAPE_LL_TRACE, "QBUS", "engines", "release engine");
        
    if (self->pvd2.ctx_del)
    {
      self->pvd2.ctx_del (&(self->ctx));
    }
    
    cape_dl_del (&(self->hlib));
    
    CAPE_DEL (p_self, struct QBusEnginesPvd_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd__on_route_request (QBusEnginesPvd self, QBusPvdConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "route [REQ] << module = %s, sender = %s", frame->module, frame->sender);

  if (cape_str_empty (frame->module))
  {
    // old version
    qbus_frame_set_type (frame, QBUS_FRAME_TYPE_ROUTE_RES, qbus_route_name_get (self->route));
  }
  else
  {
    // reset the type and the sender
    qbus_frame_set_type (frame, QBUS_FRAME_TYPE_ROUTE_RES, qbus_route_uuid_get (self->route));
    
    // reset the module
    // -> module is used as name for the module
    cape_str_replace_cp (&(frame->module), qbus_route_name_get (self->route));
  }
  
  qbus_route_send_response (self->route, frame, conn);
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd__on_route_response (QBusEnginesPvd self, QBusPvdConnection conn, QBusFrame frame)
{
  CapeUdc route_nodes = qbus_frame_get_udc (frame);
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "route [RES] << module = %s, sender = %s", frame->module, frame->sender);
  
  if (cape_str_equal (qbus_route_name_get (self->route), frame->module))
  {
    // support old version
    // -> old version has a simple relay mechanism
    qbus_route_add (self->route, frame->sender, NULL, conn, &route_nodes);
  }
  else
  {
    qbus_route_add (self->route, frame->module, frame->sender, conn, &route_nodes);
  }
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd__on_route_update (QBusEnginesPvd self, QBusPvdConnection conn, QBusFrame frame)
{
  CapeUdc route_nodes = qbus_frame_get_udc (frame);

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "route [UPD] << module = %s, sender = %s", frame->module, frame->sender);
  
  if (cape_str_equal (qbus_route_name_get (self->route), frame->module))
  {
    // support old version
    // -> old version has a simple relay mechanism
    qbus_route_add (self->route, frame->sender, NULL, conn, &route_nodes);
  }
  else
  {
    qbus_route_add (self->route, frame->module, frame->sender, conn, &route_nodes);
  }  
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd__on_obsvbl_value (QBusEnginesPvd self, QBusPvdConnection conn, QBusFrame frame)
{
  //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "obsvbl", "obsvbl [VAL] << module = %s, sender = %s", frame->module, frame->sender);

  if (cape_str_equal (frame->module, qbus_route_uuid_get (self->route)))
  {
    CapeUdc value = qbus_frame_get__payload (frame);
        
    qbus_obsvbl_value (self->obsvbl, frame->chain_key, frame->method, &value);
  }
  else
  {
    //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "obsvbl", "obsvbl [VAL] >>> forward");
    
  }  
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd__on_msg_request (QBusEnginesPvd self, QBusPvdConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  // check if the message was sent to us
  if (cape_str_compare (frame->module, qbus_route_name_get (self->route)))
  {
    qbus_methods_recv_request (self->methods, p_frame, conn, qbus_route_name_get (self->route));
  }
  else  // the message was not send to us -> forward it 
  {
    // try to find a connection which might reach the destination module
    QBusPvdConnection conn_forward = qbus_route_get (self->route, frame->module);
    if (conn_forward)
    {
      qbus_methods_recv_forward (self->methods, frame, conn_forward, qbus_route_name_get (self->route));
    }
    else
    {
      qbus_route_send_error (self->route, frame, conn);
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd__on_msg_response (QBusEnginesPvd self, QBusPvdConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  //printf ("received response to = %s -> on module = %s <--- from %s\n", frame->module, qbus_route_name_get (self->route), frame->sender);
  
  // the response is assigned by chain key
  qbus_methods_recv_response (self->methods, p_frame, conn, qbus_route_name_get (self->route));
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd__on_methods (QBusEnginesPvd self, QBusPvdConnection conn, QBusFrame frame)
{
  if (cape_str_compare (frame->module, qbus_route_uuid_get (self->route)))
  {
    qbus_methods_recv_methods (self->methods, frame, conn, qbus_route_name_get (self->route));    
  }
  else
  {
    // try to find a connection which might reach the destination module
    QBusPvdConnection conn_forward = qbus_route_get (self->route, frame->module);
    if (conn_forward)
    {
      qbus_methods_recv_forward (self->methods, frame, conn_forward, qbus_route_name_get (self->route));
    }
    else
    {
      qbus_route_send_error (self->route, frame, conn);
    }
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engines_pvd__on_frame (void* factory_ptr, QBusPvdConnection conn, QBusFrame* p_frame)
{
  QBusEnginesPvd self = factory_ptr;
  
  QBusFrame frame = *p_frame;
  
  switch (frame->ftype)
  {
    case QBUS_FRAME_TYPE_ROUTE_REQ:
    {
      qbus_engines_pvd__on_route_request (self, conn, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_ROUTE_RES:
    {
      qbus_engines_pvd__on_route_response (self, conn, frame);
      break;
    }
    case QBUS_FRAME_TYPE_ROUTE_UPD:
    {
      qbus_engines_pvd__on_route_update (self, conn, frame);
      break;
    }
    case QBUS_FRAME_TYPE_OBSVBL_VALUE:
    {
      qbus_engines_pvd__on_obsvbl_value (self, conn, frame);
      break;
    }
    case QBUS_FRAME_TYPE_MSG_REQ:
    {
      qbus_engines_pvd__on_msg_request (self, conn, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_MSG_RES:
    {
      qbus_engines_pvd__on_msg_response (self, conn, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_METHODS:
    {
      qbus_engines_pvd__on_methods (self, conn, frame);
      break;
    }
  }
  
  qbus_frame_del (p_frame);
}

//-----------------------------------------------------------------------------

QBusPvdFcts __STDCALL qbus_engines_pvd__fcts_new (void* factory_ptr, void* conecction_ptr)
{
  QBusEnginesPvd self = factory_ptr;
  
  QBusPvdFcts fcts = CAPE_NEW (struct QBusPvdFcts_s);
  
  // assign callbacks
  fcts->on_frame = qbus_engines_pvd__on_frame;
  
  // create the user object as connection context
  fcts->conn = CAPE_NEW (struct QBusPvdConnection_s);

  fcts->conn->connection_ptr = conecction_ptr;
  fcts->conn->engine = self;
  fcts->conn->version = 0;
  
  cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "entity", "new entity connection");
  
  // start handshaking - routing
  {
    QBusFrame frame = qbus_frame_new ();
    
    // CH01: replace self->name with self->uuid and add name as module
    qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_REQ, NULL, qbus_route_name_get (self->route), NULL, qbus_route_uuid_get (self->route));

    // for debug
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engines", "route [REQ] >> module = %s, sender = %s", frame->module, frame->sender);
    
    // send the frame
    self->pvd2.ctx_send (self->ctx, conecction_ptr, frame);
    
    qbus_frame_del (&frame);
  }
  
  return fcts;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engines_pvd__fcts_del (void* factory_ptr, int reconnect, QBusPvdFcts* p_self)
{
  QBusEnginesPvd engine = factory_ptr;
    
  if (*p_self)
  {
    QBusPvdFcts self = *p_self;
    
    cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "entity", "lost connection");
    
    if (reconnect)
    {
      // remove all nodes which have this connection ptr
      qbus_route_rm (engine->route, self->conn);
    }
    
    // tell methods to drop all ongoing requests to this connection
    qbus_methods_drop_conn (engine->methods, self->conn);
    
    
    CAPE_DEL (&(self->conn), struct QBusPvdConnection_s);
    
    CAPE_DEL (p_self, struct QBusPvdFcts_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_engines_pvd_load (QBusEnginesPvd self, const CapeString path, const CapeString name, CapeAioContext aio_context, CapeErr err)
{
  int res;
  
  CapeString path_current = NULL;
  CapeString path_resolved = NULL;
  CapeString pvd_name = NULL;
  
  // fetch the current path
  path_current = cape_fs_path_current (path);
  if (path_current == NULL)
  {
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "can't find path: %s", path_current);
    goto exit_and_cleanup;
  }
  
  path_resolved = cape_fs_path_resolve (path_current, err);
  if (path_resolved == NULL)
  {
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "can't find path: %s", path_current);
    goto exit_and_cleanup;
  }
  
  pvd_name = cape_str_catenate_2 ("qbus_pvd2_", name);
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "try to load engine = %s, library = %s", name, pvd_name);
  
  // try to load the module
  res = cape_dl_load (self->hlib, path_resolved, pvd_name, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  self->pvd2.ctx_new = cape_dl_funct (self->hlib, "pvd2_ctx_new", err);
  if (self->pvd2.ctx_new == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->pvd2.ctx_del = cape_dl_funct (self->hlib, "pvd2_ctx_del", err);
  if (self->pvd2.ctx_del == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->pvd2.ctx_reg = cape_dl_funct (self->hlib, "pvd2_ctx_reg", err);
  if (self->pvd2.ctx_reg == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->pvd2.ctx_send = cape_dl_funct (self->hlib, "pvd2_ctx_send", err);
  if (self->pvd2.ctx_send == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->ctx = self->pvd2.ctx_new (aio_context, qbus_engines_pvd__fcts_new, qbus_engines_pvd__fcts_del, self, err);
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_str_del (&path_current);
  cape_str_del (&path_resolved);
  cape_str_del (&pvd_name);

  return res;
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd_send (QBusEnginesPvd self, QBusFrame frame, void* connection_ptr)
{
  //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engines send", "send frame to -> %p", connection_ptr);

  // send the frame
  self->pvd2.ctx_send (self->ctx, connection_ptr, frame);
}

//-----------------------------------------------------------------------------

int qbus_engines_pvd__entity_new (QBusEnginesPvd self, const CapeUdc config, CapeErr err)
{
  self->pvd2.ctx_reg (self->ctx, config);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------
