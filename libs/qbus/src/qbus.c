#include "qbus.h" 
#include "qbus_route.h"
#include "qbus_types.h"

// c includes
#include <stdlib.h>
#include <stdio.h>

#ifdef __WINDOWS_OS

#include <windows.h>

#else

#include <termios.h>
#include <unistd.h>

#endif

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_types.h"
#include "sys/cape_file.h"
#include "sys/cape_dl.h"
#include "stc/cape_str.h"
#include "stc/cape_map.h"
#include "aio/cape_aio_sock.h"
#include "fmt/cape_args.h"
#include "fmt/cape_json.h"
#include "fmt/cape_tokenizer.h"
#include "sys/cape_btrace.h"
#include "sys/cape_thread.h"

// engines
#include "../engines/tcp/engine_tcp.h"

//-----------------------------------------------------------------------------

struct QBusEngine_s
{
  // stores the function pointers
  QbusPvd functions;
  CapeDl hlib;
  
}; typedef struct QBusEngine_s* QBusEngine;

//-----------------------------------------------------------------------------

QBusEngine qbus_engine_new ()
{
  QBusEngine self = CAPE_NEW(struct QBusEngine_s);

  memset (&(self->functions), 0, sizeof(QbusPvd));
  self->hlib = cape_dl_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_engine_del (QBusEngine* p_self)
{
  if (*p_self)
  {
    QBusEngine self = *p_self;

    cape_dl_del (&(self->hlib));
    
    CAPE_DEL (p_self, struct QBusEngine_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_engine_load (QBusEngine self, const CapeString path, const CapeString name, CapeErr err)
{
  int res;
  
  CapeString path_current = NULL;
  CapeString path_resolved = NULL;
  CapeString pvd_name = NULL;
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "try to load engine = %s", name);

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
  
  pvd_name = cape_str_catenate_2 ("qbus_pvd_", name);
  
  // try to load the module
  res = cape_dl_load (self->hlib, path_resolved, pvd_name, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->functions.pvd_ctx_new = cape_dl_funct (self->hlib, "qbus_pvd_ctx_new", err);
  if (self->functions.pvd_ctx_new == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_ctx_del = cape_dl_funct (self->hlib, "qbus_pvd_ctx_del", err);
  if (self->functions.pvd_ctx_del == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_ctx_add = cape_dl_funct (self->hlib, "qbus_pvd_ctx_add", err);
  if (self->functions.pvd_ctx_add == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_ctx_rm = cape_dl_funct (self->hlib, "qbus_pvd_ctx_rm", err);
  if (self->functions.pvd_ctx_rm == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_listen = cape_dl_funct (self->hlib, "qbus_pvd_listen", err);
  if (self->functions.pvd_listen == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_reconnect = cape_dl_funct (self->hlib, "qbus_pvd_reconnect", err);
  if (self->functions.pvd_reconnect == NULL)
  {
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "functions mapped = %s", name);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_str_del (&path_current);
  cape_str_del (&path_resolved);
  cape_str_del (&pvd_name);

  return res;
}

//-----------------------------------------------------------------------------

QbusPvdCtx qbus_engine_ctx_new (QBusEngine self, CapeAioContext aio, CapeUdc options, CapeErr err)
{
  QbusPvdCtx ret = NULL;
  
  if (self->functions.pvd_ctx_new)
  {
    ret = self->functions.pvd_ctx_new (aio, options, err);
  }
  else
  {
    cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void* __STDCALL qbus_engine_conn__factory_on_new (void* user_ptr)
{
  
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine_conn__factory_on_del (void** object_ptr)
{
  
}

//-----------------------------------------------------------------------------

void* __STDCALL qbus_engine_conn__on_connection (void* object_ptr)
{
  
}

//-----------------------------------------------------------------------------

QbusPvdConn qbus_engine_conn_new (QBusEngine self, QbusPvdCtx ctx, CapeUdc options, CapeErr err)
{
  QbusPvdConn ret = NULL;
  
  if (self->functions.pvd_ctx_add)
  {
    ret = self->functions.pvd_ctx_add (ctx, options, self, qbus_engine_conn__factory_on_new, qbus_engine_conn__factory_on_del, qbus_engine_conn__on_connection);
  }
  else
  {
    cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

int qbus_engine_reconnect (QBusEngine self, QbusPvdConn conn, CapeErr err)
{
  int res;
  
  if (self->functions.pvd_reconnect)
  {
    res = self->functions.pvd_reconnect (conn, err);
  }
  else
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface can't reconnect");
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engines__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusEngine h = val; qbus_engine_del (&h);
  }
}

//-----------------------------------------------------------------------------

struct QBus_s
{
  CapeAioContext aio;
  
  CapeString name;
  
  QBusRoute route;
  
  // TODO -> into list
  EngineTcpInc engine_tcp_inc;
  
  // TODO -> into list
  EngineTcpOut engine_tcp_out;
  
  // config
  CapeUdc config;
  
  CapeString config_file;
  
  CapeMap engines;
};

//-----------------------------------------------------------------------------

QBus qbus_new (const char* module_origin)
{
  QBus self = CAPE_NEW(struct QBus_s);

  // create an upper name
  self->name = cape_str_cp (module_origin);  
  cape_str_to_upper (self->name);
  
  self->route = qbus_route_new (self, self->name);
  
  self->aio = cape_aio_context_new ();
  
  self->engine_tcp_inc = NULL;
  self->engine_tcp_out = NULL;
  
  self->config = NULL;
  self->config_file = NULL;
  
  self->engines = cape_map_new (NULL, qbus_engines__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_del (QBus* p_self)
{
  QBus self = *p_self;
  
  cape_aio_context_del (&(self->aio));
  
  cape_str_del (&(self->name));
  
  qbus_engine_tcp_inc_del (&(self->engine_tcp_inc));
  qbus_engine_tcp_out_del (&(self->engine_tcp_out));
  
  qbus_route_del (&(self->route));
  
  cape_udc_del (&(self->config));
  cape_str_del (&(self->config_file));
  
  cape_map_del (&(self->engines));
  
  CAPE_DEL (p_self, struct QBus_s);
}

//-----------------------------------------------------------------------------

void qbus_add_income_port (QBus self, CapeUdc bind)
{
  const CapeString type = cape_udc_get_s (bind, "type", NULL);
  
  if (type == NULL)
  {
    return;
  }
  
  if (strcmp (type, "pipe") == 0)
  {
    // check if we have name and path
    const CapeString name = cape_udc_get_s (bind, "name", NULL);
    const CapeString path = cape_udc_get_s (bind, "path", NULL);
    
    if (name && path)
    {
      
    }
    
    return;
  }
  
  if (strcmp (type, "socket") == 0)
  {
    // check if we have host and port
    const CapeString host = cape_udc_get_s (bind, "host", NULL);
    number_t port = cape_udc_get_n (bind, "port", 0);
    
    if (host && port)
    {
      self->engine_tcp_inc = qbus_engine_tcp_inc_new (self->aio, self->route, host, port);

      // power up engine
      {
        CapeErr err = cape_err_new ();
        
        int res = qbus_engine_tcp_inc_listen (self->engine_tcp_inc, err);
        if (res)
        {
          cape_log_fmt (CAPE_LL_ERROR, "QBUS", "add income", "error in listen: %s", cape_err_text (err));
        }
        
        cape_err_del (&err);
      }
    }
    
    return;
  }
}

//-----------------------------------------------------------------------------

QBusEngine qbus_load_engine__library (QBus self, const CapeString path, const CapeString name, CapeErr err)
{
  QBusEngine ret = NULL;
  
  CapeMapNode n = cape_map_find (self->engines, (void*)name);
  if (n)
  {
    ret = cape_map_node_value (n);
  }
  else
  {
    ret = qbus_engine_new ();

    // try to load the engine
    // -> returns a cape error code
    if (qbus_engine_load (ret, path, name, err))
    {
      // cleanup
      qbus_engine_del (&ret);
    }
    else
    {
      // append the engine to the engines map
      cape_map_insert (self->engines, (void*)cape_str_cp (name), ret);
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

typedef struct {
  
  QbusPvdConn conn;
  QBusEngine engine;
  
} QbusEngineResult;

//-----------------------------------------------------------------------------

int qbus_load_engine (QBus self, const CapeString name, CapeUdc remote, QbusEngineResult* result, CapeErr err)
{
  int res;
  const CapeString path = "qbus";

  // local objects
  QbusPvdCtx ctx = NULL;

  // load and register the engine
  result->engine = qbus_load_engine__library (self, path, name, err);
  if (result->engine == NULL)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "engine", "error in loading the engine = %s", name);
    
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  // TODO: save the context into a list
  ctx = qbus_engine_ctx_new (result->engine, self->aio, NULL, err);
  if (ctx == NULL)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "engine", "error in initializing the engine = %s", name);
    
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  result->conn = qbus_engine_conn_new (result->engine, ctx, remote, err);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_add_remote_port (QBus self, CapeUdc remote, CapeErr err)
{
  int res;
  
  const CapeString type = cape_udc_get_s (remote, "type", NULL);
  
  if (type == NULL)
  {
    return CAPE_ERR_NONE;
  }
  
  if (strcmp (type, "pipe") == 0)
  {
    // check if we have name and path
    const CapeString name = cape_udc_get_s (remote, "name", NULL);
    const CapeString path = cape_udc_get_s (remote, "path", NULL);
    
    if (name && path)
    {
      
    }
    
    return CAPE_ERR_NONE;
  }
  else if (strcmp (type, "socket") == 0)
  {
    // check if we have host and port
    const CapeString host = cape_udc_get_s (remote, "host", NULL);
    number_t port = cape_udc_get_n (remote, "port", 0);
    
    if (host && port)
    {
      self->engine_tcp_out = qbus_engine_tcp_out_new (self->aio, self->route, host, port);
      
      // power up engine
      {
        CapeErr err = cape_err_new ();
        
        int res = qbus_engine_tcp_out_reconnect (self->engine_tcp_out, err);
        if (res)
        {
          cape_log_fmt (CAPE_LL_ERROR, "QBUS", "add remote", "error in connect: %s", cape_err_text (err));
        }
        
        cape_err_del (&err);
      }
    }
    
    return CAPE_ERR_NONE;
  }
  else if (strcmp (type, "tcp") == 0)
  {
    QbusEngineResult result;
    
    res = qbus_load_engine (self, "tcp", remote, &result, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    res = qbus_engine_reconnect (result.engine, result.conn, err);
  }
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_add_income_ports (QBus self, CapeUdc binds)
{
  switch (cape_udc_type (binds))
  {
    case CAPE_UDC_LIST:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (binds, CAPE_DIRECTION_FORW);
      
      while (cape_udc_cursor_next (cursor))
      {
        qbus_add_income_port (self, cursor->item);
      }
      
      cape_udc_cursor_del (&cursor);
      
      break;
    }
    case CAPE_UDC_NODE:
    {
      qbus_add_income_port (self, binds);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_add_remote_ports (QBus self, CapeUdc remotes, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdcCursor* cursor = NULL;
  
  switch (cape_udc_type (remotes))
  {
    case CAPE_UDC_LIST:
    {
      cursor = cape_udc_cursor_new (remotes, CAPE_DIRECTION_FORW);
      
      while (cape_udc_cursor_next (cursor))
      {
        res = qbus_add_remote_port (self, cursor->item, err);
        if (res)
        {
          goto exit_and_cleanup;
        }
      }
      
      res = CAPE_ERR_NONE;
      break;
    }
    case CAPE_UDC_NODE:
    {
      res = qbus_add_remote_port (self, remotes, err);
      break;
    }
  }
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  return res;
}

//-----------------------------------------------------------------------------

int qbus_wait__intern (QBus self, CapeUdc binds, CapeUdc remotes, CapeErr err)
{
  int res;
  
  // activate signal handling strategy
  res = cape_aio_context_set_interupts (self->aio, TRUE, TRUE, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  if (binds)
  {
    qbus_add_income_ports (self, binds);
  }
  
  if (remotes)
  {
    res = qbus_add_remote_ports (self, remotes, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  // wait infinite and let the AIO subsystem handle all events
  res = cape_aio_context_wait (self->aio, err);
  
exit_and_cleanup:
  
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "wait", "runtime error: %s", cape_err_text (err));
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_wait (QBus self, CapeUdc binds, CapeUdc remotes, number_t workers, CapeErr err)
{
  int res;
  
  res = qbus_route_init (self->route, workers, err);
  if (res)
  {
    return res;
  }

  // open the operating system AIO/event subsystem
  res = cape_aio_context_open (self->aio, err);
  if (res)
  {
    return res;
  }
  
  return qbus_wait__intern (self, binds, remotes, err);
}

//-----------------------------------------------------------------------------

int qbus_register (QBus self, const char* method, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm, CapeErr err)
{
  qbus_route_meth_reg (self->route, method, ptr, onMsg, onRm);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_send (QBus self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg, CapeErr err)
{
  // correct chain key
  // -> the key for the start must be NULL
  cape_str_del (&(msg->chain_key));
  
  qbus_route_request (self->route, module, method, msg, ptr, onMsg, FALSE, err);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_test_s (QBus self, const char* module, const char* method, CapeErr err)
{  
    /*
  QBusM message = qbus_message_new(NULL, NULL);
  message->cdata = cape_json_from_s(request);
  qbus_route_request (self->route, module, method, message, ptr, onMsg, FALSE);
  */

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_continue (QBus self, const char* module, const char* method, QBusM qin, void** p_ptr, fct_qbus_onMessage on_msg, CapeErr err)
{
  int res;
  
  if (p_ptr)
  {
    res = qbus_route_request (self->route, module, method, qin, *p_ptr, on_msg, TRUE, err);

    *p_ptr = NULL;
  }
  else
  {
    res = qbus_route_request (self->route, module, method, qin, NULL, on_msg, TRUE, err);
  }
    
  return res;
}

//-----------------------------------------------------------------------------

int qbus_response (QBus self, const char* module, QBusM msg, CapeErr err)
{
  qbus_route_response (self->route, module, msg, NULL);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

const CapeString qbus_name (QBus self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

CapeAioContext qbus_aio (QBus self)
{
  return self->aio;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_modules (QBus self)
{
  return qbus_route_modules (self->route);
}

//-----------------------------------------------------------------------------

QBusConnection const qbus_find_conn (QBus self, const char* module)
{
  return qbus_route_module_find (self->route, module);
}

//-----------------------------------------------------------------------------

void qbus_conn_request (QBus self, QBusConnection const conn, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  qbus_route_conn_request (self->route, conn, module, method, msg, ptr, onMsg, FALSE);
}

//-----------------------------------------------------------------------------

QBusM qbus_message_new (const CapeString key, const CapeString sender)
{
  QBusM self = CAPE_NEW (struct QBusMessage_s);
  
  if (key)
  {
    // clone the key
    self->chain_key = cape_str_cp (key);
  }
  else
  {
    // create a new key
    self->chain_key = cape_str_uuid ();    
  }
  
  self->sender = cape_str_cp (sender);
  
  // init the objects
  self->cdata = NULL;
  self->pdata = NULL;
  self->clist = NULL;
  self->rinfo = NULL;
  self->files = NULL;
  self->blob = NULL;
  
  self->err = NULL;
  
  self->mtype = QBUS_MTYPE_NONE;
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_message_clr (QBusM self, u_t cdata_udc_type)
{
  cape_udc_del (&(self->cdata));
  cape_udc_del (&(self->pdata));
  cape_udc_del (&(self->clist));
  cape_stream_del (&(self->blob));
  
  cape_err_del (&(self->err));
  
  if (cdata_udc_type != CAPE_UDC_UNDEFINED)
  {
    // we don't need a name
    self->cdata = cape_udc_new (cdata_udc_type, NULL);
  }
}

//-----------------------------------------------------------------------------

void qbus_message_del (QBusM* p_self)
{
  QBusM self = *p_self;
  
  qbus_message_clr (self, CAPE_UDC_UNDEFINED);

  // only clear it here
  cape_udc_del (&(self->rinfo));
  cape_udc_del (&(self->files));

  cape_str_del (&(self->chain_key));
  cape_str_del (&(self->sender));
  
  CAPE_DEL (p_self, struct QBusMessage_s);
}

//-----------------------------------------------------------------------------

QBusM qbus_message_data_mv (QBusM source)
{
  QBusM self = CAPE_NEW (struct QBusMessage_s);

  self->chain_key = cape_str_cp (source->chain_key);
  self->sender = cape_str_cp (source->sender);

  // move the objects
  self->cdata = cape_udc_mv (&(source->cdata));
  self->pdata = cape_udc_mv (&(source->pdata));
  self->clist = cape_udc_mv (&(source->clist));
  self->rinfo = cape_udc_mv (&(source->rinfo));
  self->files = cape_udc_mv (&(source->files));
  
  if (source->blob)
  {
    self->blob = source->blob;
    source->blob = NULL;
  }
  
  self->err = NULL;
  self->mtype = source->mtype;
  
  return self;
}

//-----------------------------------------------------------------------------

int qbus_message_role_has (QBusM self, const CapeString role_name)
{
  CapeUdc roles;
  CapeUdc role;

  if (self->rinfo == NULL)
  {
    return FALSE;
  }
  
  roles = cape_udc_get (self->rinfo, "roles");
  if (roles == NULL)
  {
    return FALSE;
  }

  role = cape_udc_get (roles, role_name);
  if (role == NULL)
  {
    return FALSE;
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------

int qbus_message_role_or2 (QBusM self, const CapeString role01, const CapeString role02)
{
  CapeUdc roles;
  CapeUdc role;
  
  if (self->rinfo == NULL)
  {
    cape_log_msg (CAPE_LL_WARN, "QBUS", "role or2", "rinfo is NULL");
    return FALSE;
  }
  
  roles = cape_udc_get (self->rinfo, "roles");
  if (roles == NULL)
  {
    return FALSE;
  }

  role = cape_udc_get (roles, role01);
  if (role == NULL)
  {
    role = cape_udc_get (roles, role02);
    if (role == NULL)
    {
      return FALSE;
    }
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------

void qbus_check_param__string (CapeUdc data, const CapeUdc string_param)
{
  const CapeString h = cape_udc_s (string_param, NULL);
  if (h)
  {
    CapeList options = cape_tokenizer_buf (h, strlen(h), ':');
    
    if (cape_list_size(options) == 3)
    {
      CapeUdc n = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      CapeListCursor* cursor = cape_list_cursor_create (options, CAPE_DIRECTION_FORW);
      
      if (cape_list_cursor_next (cursor))
      {
        cape_udc_add_s_cp (n, "type", cape_list_node_data (cursor->node));
      }
      
      if (cape_list_cursor_next (cursor))
      {
        cape_udc_add_s_cp (n, "host", cape_list_node_data (cursor->node));
      }
      
      if (cape_list_cursor_next (cursor))
      {
        cape_udc_add_n (n, "port", strtol (cape_list_node_data (cursor->node), NULL, 10));
      }
      
      cape_list_cursor_destroy (&cursor);
      
      cape_udc_add (data, &n);
    }
    
    cape_list_del (&options);
  }
}

//-----------------------------------------------------------------------------

void qbus_check_param (CapeUdc data, const CapeUdc param)
{
  switch (cape_udc_type (param))
  {
    case CAPE_UDC_NODE:
    {
      CapeUdc h = cape_udc_cp (param);
      cape_udc_add (data, &h);
      break;
    }
    case CAPE_UDC_LIST:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (param, CAPE_DIRECTION_FORW);

      while (cape_udc_cursor_next (cursor))
      {
        qbus_check_param (data, cursor->item);
      }
      
      cape_udc_cursor_del (&cursor);
      break;
    }
    case CAPE_UDC_STRING:
    {
      qbus_check_param__string (data, param);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_config_load__find_file (QBus self, CapeErr err)
{
  CapeString filename = cape_str_catenate_2 (self->name, ".json");

  self->config_file = cape_args_config_file ("etc", filename);
  
  cape_str_del (&filename);
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_load__local (QBus self)
{
  CapeUdc ret = NULL;
  CapeErr err = cape_err_new ();

  // create or find the config file
  qbus_config_load__find_file (self, err);
  
  if (self->config_file)
  {
    // try to load
    ret = cape_json_from_file (self->config_file, err);
  }
  
  if (cape_err_code (err))
  {
    // dump error text
    cape_log_msg (CAPE_LL_ERROR, self->name, "config load", cape_err_text (err));
  }
  
  cape_err_del (&err);
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_load__global (QBus self)
{
  CapeUdc ret = NULL;

  // local objects
  CapeErr err = cape_err_new ();
  CapeString global_config_file = cape_args_config_file ("etc", "qbus_default.json");

  if (global_config_file)
  {
    // try to load
    ret = cape_json_from_file (global_config_file, err);
  }
  
  if (cape_err_code (err))
  {
    // dump error text
    cape_log_msg (CAPE_LL_ERROR, self->name, "config load", cape_err_text (err));
  }

  cape_str_del (&global_config_file);
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_config_save (QBus self, CapeUdc config)
{
  // try to load
  if (self->config && cape_udc_size (config))
  {
    int res;
    CapeErr err = cape_err_new ();
    
    res = cape_json_to_file (self->config_file, config, err);
    if (res)
    {
      // dump error text
      cape_log_fmt (CAPE_LL_ERROR, self->name, "config save: %s", cape_err_text (err), self->config_file);
    }
    
    cape_err_del (&err);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_config_s (QBus self, const char* name, const CapeString default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_s (config_node, default_val);
  }
  else
  {
    cape_udc_add_s_cp (self->config, name, default_val);

    return default_val;
  }
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_node (QBus self, const char* name)
{
  CapeUdc config_node;

  if (self->config == NULL)
  {
    return NULL;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return config_node;
  }
  else
  {
    return cape_udc_add_node (self->config, name);
  }
}

//-----------------------------------------------------------------------------

number_t qbus_config_n (QBus self, const char* name, number_t default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_n (config_node, default_val);
  }
  else
  {    
    cape_udc_add_n (self->config, name, default_val);

    return default_val;
  }
}

//-----------------------------------------------------------------------------

double qbus_config_f (QBus self, const char* name, double default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_f (config_node, default_val);
  }
  else
  {    
    cape_udc_add_f (self->config, name, default_val);
    
    return default_val;
  }
}

//-----------------------------------------------------------------------------

int qbus_config_b (QBus self, const char* name, int default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_b (config_node, default_val);
  }
  else
  {    
    cape_udc_add_b (self->config, name, default_val);
    
    return default_val;
  }
}

//-----------------------------------------------------------------------------

void qbus_instance (const char* name, void* ptr, fct_qbus_on_init on_init, fct_qbus_on_done on_done, int argc, char *argv[])
{
  int res = CAPE_ERR_NONE;

  const CapeString module_name;
  
  // local objects
  CapeErr err = cape_err_new ();
  CapeFileLog log = NULL;

  QBus self = NULL;

  CapeUdc bind = NULL;
  CapeUdc remotes = NULL;
  CapeUdc args = NULL;
  
  void* user_ptr= NULL;
  
  printf ("    ooooooo  oooooooooo ooooo  oooo oooooooo8  \n");
  printf ("  o888   888o 888    888 888    88 888         \n");
  printf ("  888     888 888oooo88  888    88  888oooooo  \n");
  printf ("  888o  8o888 888    888 888    88         888 \n");
  printf ("    88ooo88  o888ooo888   888oo88  o88oooo888  \n");
  printf ("         88o8                                  \n");
  printf ("\n");

  res = cape_btrace_activate (err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // convert program arguments into a node with parameters
  args = cape_args_from_args (argc, argv, NULL);

  // replace name from the parameters
  module_name = cape_udc_get_s (args, "n", name);
  
  // start a new qbus instance
  self = qbus_new (module_name);
  
  // create params
  self->config = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  if (args)
  {
    cape_udc_merge_mv (self->config, &args);
  }
  
  {
    // load config from local file
    CapeUdc config_part = qbus_config_load__local (self);

    if (config_part)
    {
      // save the config back
      qbus_config_save (self, config_part);

      cape_udc_merge_mv (self->config, &config_part);
    }
  }

  {
    // load config from a global file
    CapeUdc config_part = qbus_config_load__global (self);

    if (config_part)
    {
      cape_udc_merge_mv (self->config, &config_part);
    }
  }

  // filelogging
  {
    CapeUdc arg_l = cape_udc_get (self->config, "log_file");
   
    if (arg_l) switch (cape_udc_type (arg_l))
    {
      case CAPE_UDC_STRING:
      {
        log = cape_log_new (cape_udc_s (arg_l, NULL));
        break;
      }
    }
  }
  
  // debug output
  {
    // use this as default
    CapeLogLevel log_level = CAPE_LL_INFO;
    
    CapeUdc arg_d = cape_udc_get (self->config, "log_level");
    
    if (arg_d) switch (cape_udc_type (arg_d))
    {
      case CAPE_UDC_STRING:
      {
        log_level = cape_log_level_from_s (cape_udc_s (arg_d, NULL), log_level);
        break;
      }
      case CAPE_UDC_NUMBER:
      {
        log_level = cape_udc_n (arg_d, log_level);
        break;
      }
    }
    
    // adjust the global log level
    cape_log_set_level (log_level);
  }
  
  // debug
  {
    CapeString h = cape_json_to_s (self->config);
    
    cape_log_fmt (CAPE_LL_INFO, name, "qbus_instance", "params: %s", h);
    
    cape_str_del (&h);
  }
  
  // check for remotes
  {
    CapeUdc arg_r = cape_udc_get (self->config, "d");
    if (arg_r)
    {
      remotes = cape_udc_new (CAPE_UDC_LIST, NULL);
      
      qbus_check_param (remotes, arg_r);
    }
  }
      
  // check for binds
  {
    CapeUdc arg_b = cape_udc_get (self->config, "b");
    if (arg_b)
    {
      bind = cape_udc_new (CAPE_UDC_LIST, NULL);
      
      qbus_check_param (bind, arg_b);
    }
  }
      
  cape_log_msg (CAPE_LL_TRACE, name, "qbus_instance", "arguments parsed");
  
  // open the operating system AIO/event subsystem
  res = cape_aio_context_open (self->aio, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "instance", "error in initialization: %s", cape_err_text(err));    
    goto exit_and_cleanup;
  }
  
  res = qbus_route_init (self->route, cape_udc_get_n (self->config, "threads", cape_thread_concurrency () * 2), err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (on_init)
  {
    res = on_init (self, ptr, &user_ptr, err);
  }

  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "instance", "error in initialization: %s", cape_err_text(err));    
    goto exit_and_cleanup;
  }
  
  cape_log_msg (CAPE_LL_TRACE, name, "qbus_instance", "start main loop");

#if defined __WINDOWS_OS

  // TODO: nice to have
  {


  }

#else

  // disable console echoing
  {
    struct termios saved;
    struct termios attributes;
    
    tcgetattr(STDIN_FILENO, &saved);
    tcgetattr(STDIN_FILENO, &attributes);
    
    attributes.c_lflag &= ~ ECHO;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attributes);    

    // *** main loop ***
    qbus_wait__intern (self, bind, remotes, err);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
  }

#endif

  if (on_done)
  {
    on_done (self, user_ptr, err);
  }
  
exit_and_cleanup:
  
  cape_udc_del (&bind);
  cape_udc_del (&remotes);
  
  qbus_del (&self);
  
  cape_err_del (&err);
  cape_log_del (&log);
}

//-----------------------------------------------------------------------------

void* qbus_add_on_change (QBus self, void* ptr, fct_qbus_on_route_change on_change)
{
  return qbus_route_add_on_change (self->route, ptr, on_change);
}

//-----------------------------------------------------------------------------

void qbus_rm_on_change (QBus self, void* obj)
{
  qbus_route_rm_on_change (self->route, obj);
}

//-----------------------------------------------------------------------------

void qbus_methods (QBus self, const char* module, void* ptr, fct_qbus_on_methods on_methods)
{
  qbus_route_methods (self->route, module, ptr, on_methods);
}

//-----------------------------------------------------------------------------
