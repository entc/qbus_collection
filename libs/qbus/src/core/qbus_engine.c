#include "qbus_engine.h"
#include "qbus_connection.h"

// cape includes
#include "fmt/cape_json.h"
#include "sys/cape_log.h"
#include "sys/cape_dl.h"
#include "sys/cape_file.h"

// static engines
#include "pvd_tcp/qbus_tcp.h"

//-----------------------------------------------------------------------------

struct QBusEngine_s
{
  QBusRoute route;     // reference
  
  QbusPvd functions;   // stores the function pointers
  CapeDl hlib;         // handle for the shared library
  
  CapeList ctxs;       // list of all contexts
};

//-----------------------------------------------------------------------------

typedef struct
{
  QBusEngine engine;
  QbusPvdCtx ctx;
  
} QBusEngineCtxItem;   // just a helper struct to hold 2 values

//-----------------------------------------------------------------------------

int qbus_engine_ctx_del (QBusEngine self, QbusPvdCtx* p_ctx, CapeErr err);

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine_ctxs__on_del (void* ptr)
{
  QBusEngineCtxItem* item = ptr;
  CapeErr err = cape_err_new();
  
  qbus_engine_ctx_del (item->engine, &(item->ctx), err);
  CAPE_DEL (&item, QBusEngineCtxItem);
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

QBusEngine qbus_engine_new (QBusRoute route)
{
  QBusEngine self = CAPE_NEW(struct QBusEngine_s);
  
  self->route = route;
  
  memset (&(self->functions), 0, sizeof(QbusPvd));
  self->hlib = cape_dl_new ();
  
  self->ctxs = cape_list_new (qbus_engine_ctxs__on_del);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_engine_del (QBusEngine* p_self)
{
  if (*p_self)
  {
    QBusEngine self = *p_self;
    
    if (self->functions.pvd_done)
    {
      self->functions.pvd_done ();
    }
    
    cape_dl_del (&(self->hlib));
    cape_list_del (&(self->ctxs));
    
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
  
  self->functions.pvd_send = cape_dl_funct (self->hlib, "qbus_pvd_send", err);
  if (self->functions.pvd_send == NULL)
  {
    goto exit_and_cleanup;
  }
  
  self->functions.pvd_mark = cape_dl_funct (self->hlib, "qbus_pvd_mark", err);
  if (self->functions.pvd_mark == NULL)
  {
    goto exit_and_cleanup;
  }
  
  self->functions.pvd_cb_raw_set = cape_dl_funct (self->hlib, "qbus_pvd_cb_raw_set", err);
  if (self->functions.pvd_cb_raw_set == NULL)
  {
    goto exit_and_cleanup;
  }
  
  // optional methods
  // if the library has its own initializing concept, those methods can be implemented
  self->functions.pvd_init = cape_dl_funct (self->hlib, "qbus_pvd_init", err);
  self->functions.pvd_done = cape_dl_funct (self->hlib, "qbus_pvd_done", err);

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "functions mapped = %s", name);

  if (self->functions.pvd_init)
  {
    res = self->functions.pvd_init (err);
  }
  else
  {
    res = CAPE_ERR_NONE;
  }
  
exit_and_cleanup:
  
  cape_str_del (&path_current);
  cape_str_del (&path_resolved);
  cape_str_del (&pvd_name);
  
  return res;
}

//-----------------------------------------------------------------------------

QbusPvdCtx qbus_engine_ctx_add (QBusEngine self, CapeAioContext aio, CapeUdc options, CapeErr err)
{
  QbusPvdCtx ret = NULL;
  
  if (self->functions.pvd_ctx_new)
  {
    // create a new engine context
    // -> might use the AIO for event handling
    // -> might use the options for config
    // -> contains all entities and connections
    ret = self->functions.pvd_ctx_new (aio, options, err);
    if (ret)
    {
      QBusEngineCtxItem* item = CAPE_NEW (QBusEngineCtxItem);
      
      item->engine = self;
      item->ctx = ret;
      
      // save the context in a list
      // can be destroyed later with the whole engine
      cape_list_push_back (self->ctxs, item);
    }
  }
  else
  {
    cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

int qbus_engine_ctx_del (QBusEngine self, QbusPvdCtx* p_ctx, CapeErr err)
{
  int res;
  
  if (self->functions.pvd_ctx_del)
  {
    self->functions.pvd_ctx_del (p_ctx);
    res = CAPE_ERR_NONE;
  }
  else
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine__send (void* ptr1, void* ptr2, const char* bufdat, number_t buflen, void* userdata)
{
  QBusEngine self = ptr1;
  QbusPvdConnection connection = ptr2;
  
  if (self->functions.pvd_send)
  {
    self->functions.pvd_send (connection, bufdat, buflen, userdata);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine__mark (void* ptr1, void* ptr2)
{
  QBusEngine self = ptr1;
  QbusPvdConnection connection = ptr2;
  
  if (self->functions.pvd_mark)
  {
    self->functions.pvd_mark (connection);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine__on_sent (void* ptr, CapeAioSocket socket, void* userdata)
{
  // use the connection class to handle the protocol
  qbus_connection_onSent (ptr, userdata);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine__on_recv (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen)
{
  // use the connection class to handle the protocol
  qbus_connection_onRecv (ptr, bufdat, buflen);
}

//-----------------------------------------------------------------------------

void* __STDCALL qbus_engine__factory_on_new (void* user_ptr, QbusPvdConnection phy_connection)
{
  QBusEngine self = user_ptr;
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "new connection was established");
  
  QBusConnection ret = qbus_connection_new (self->route, 0);
  
  if (self->functions.pvd_cb_raw_set)
  {
    self->functions.pvd_cb_raw_set (phy_connection, qbus_engine__on_sent, qbus_engine__on_recv);
  }
  
  qbus_connection_cb (ret, self, phy_connection, qbus_engine__send, qbus_engine__mark);
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine__factory_on_del (void** p_object_ptr)
{
  qbus_connection_del ((QBusConnection*)p_object_ptr);
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "a connection was dropped");
}

//-----------------------------------------------------------------------------

void* __STDCALL qbus_engine__on_connection (void* object_ptr)
{
  qbus_connection_reg (object_ptr);
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "connection ready for handshake");
}

//-----------------------------------------------------------------------------

QbusPvdEntity qbus_engine_add (QBusEngine self, QbusPvdCtx ctx, CapeUdc options, CapeErr err)
{
  QbusPvdEntity ret = NULL;
  
  if (self->functions.pvd_ctx_add)
  {
    ret = self->functions.pvd_ctx_add (ctx, options, self, qbus_engine__factory_on_new, qbus_engine__factory_on_del, qbus_engine__on_connection);
  }
  else
  {
    cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

int qbus_engine_reconnect (QBusEngine self, QbusPvdEntity entity, CapeErr err)
{
  int res;
  
  if (self->functions.pvd_reconnect)
  {
    res = self->functions.pvd_reconnect (entity, err);
  }
  else
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface can't reconnect");
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_engine_listen (QBusEngine self, QbusPvdEntity entity, CapeErr err)
{
  int res;
  
  if (self->functions.pvd_listen)
  {
    res = self->functions.pvd_listen (entity, err);
  }
  else
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface can't listen");
  }
  
  return res;
}

//-----------------------------------------------------------------------------

QBusEngine qbus_engine_default (QBusRoute route)
{
  QBusEngine self = qbus_engine_new (route);
  
  self->functions.pvd_ctx_new = qbus_pvd_ctx_new;
  self->functions.pvd_ctx_del = qbus_pvd_ctx_del;
  self->functions.pvd_ctx_add = qbus_pvd_ctx_add;
  self->functions.pvd_ctx_rm = qbus_pvd_ctx_rm;
  self->functions.pvd_listen = qbus_pvd_listen;
  self->functions.pvd_reconnect = qbus_pvd_reconnect;
  
  self->functions.pvd_send = qbus_pvd_send;
  self->functions.pvd_mark = qbus_pvd_mark;
  self->functions.pvd_cb_raw_set = qbus_pvd_cb_raw_set;

  return self;
}

//-----------------------------------------------------------------------------
