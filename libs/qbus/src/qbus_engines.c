#include "qbus_engines.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include "sys/cape_dl.h"

//-----------------------------------------------------------------------------
// forward function declarations

int qbus_engine_load (QBusEngine self, const CapeString path, const CapeString engine_name, CapeErr err);
int qbus_engine__lib__ctx_del (QBusEngine self, QbusPvdCtx* p_ctx, CapeErr err);

//-----------------------------------------------------------------------------

struct QBusEngineCtxItem_s
{
  QBusEngine engine;
  QbusPvdCtx ctx;

}; typedef struct QBusEngineCtxItem_s* QBusEngineCtxItem;

//-----------------------------------------------------------------------------

QBusEngineCtxItem qbus_engine_ctx_item_new (QBusEngine engine, QbusPvdCtx ctx)
{
  QBusEngineCtxItem self = CAPE_NEW (struct QBusEngineCtxItem_s);

  self->engine = engine;
  self->ctx = ctx;

  return self;
}

//-----------------------------------------------------------------------------

void qbus_engine_ctx_item_del (QBusEngineCtxItem* p_self)
{
  if (*p_self)
  {
    QBusEngineCtxItem self = *p_self;

    {
      // local objects
      CapeErr err = cape_err_new();

      if (qbus_engine__lib__ctx_del (self->engine, &(self->ctx), err))
      {
        cape_log_fmt (CAPE_LL_ERROR, "QBUS", "engine", "error = %s", cape_err_text (err));
      }

      cape_err_del (&err);
    }

    CAPE_DEL (p_self, struct QBusEngineCtxItem_s);
  }
}

//-----------------------------------------------------------------------------

struct QBusEngine_s
{
  CapeDl hlib;         // handle for the shared library
  QbusPvd functions;   // stores the function pointers

  CapeList ctxs;       // list of all contexts
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine_ctxs__on_del (void* ptr)
{
  QBusEngineCtxItem item = ptr; qbus_engine_ctx_item_del (&item);
}

//-----------------------------------------------------------------------------

QBusEngine qbus_engine_new (const CapeString path, const CapeString engine_name, CapeErr err)
{
  QBusEngine self = CAPE_NEW (struct QBusEngine_s);

  // clean function pointers
  memset (&(self->functions), 0, sizeof(QbusPvd));

  // create a new dynamic library loader
  self->hlib = cape_dl_new ();

  // create a list to hold all contextes
  self->ctxs = cape_list_new (qbus_engine_ctxs__on_del);

  // try to load the library
  if (qbus_engine_load (self, path, engine_name, err))
  {
    // in case of an error return NULL
    qbus_engine_del (&self);
  }

  return self;
}

//-----------------------------------------------------------------------------

void qbus_engine_del (QBusEngine* p_self)
{
  if (*p_self)
  {
    QBusEngine self = *p_self;

    // end all contextes
    cape_list_del (&(self->ctxs));

    if (self->functions.pvd_done)
    {
      self->functions.pvd_done ();
    }

    // clean function pointers
    memset (&(self->functions), 0, sizeof(QbusPvd));

    // unload the shared library
    cape_dl_del (&(self->hlib));

    CAPE_DEL (p_self, struct QBusEngine_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_engine_load (QBusEngine self, const CapeString path, const CapeString engine_name, CapeErr err)
{
  int res;

  CapeString path_current = NULL;
  CapeString path_resolved = NULL;
  CapeString pvd_name = NULL;

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "try to load engine = %s", engine_name);

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

  pvd_name = cape_str_catenate_2 ("qbus_pvd_", engine_name);

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

  self->functions.pvd_con_cid = cape_dl_funct (self->hlib, "qbus_pvd_con_cid", err);
  if (self->functions.pvd_con_cid == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_con_snd = cape_dl_funct (self->hlib, "qbus_pvd_con_snd", err);
  if (self->functions.pvd_con_snd == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_con_subscribe = cape_dl_funct (self->hlib, "qbus_pvd_con_subscribe", err);
  if (self->functions.pvd_con_subscribe == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_con_unsubscribe = cape_dl_funct (self->hlib, "qbus_pvd_con_unsubscribe", err);
  if (self->functions.pvd_con_unsubscribe == NULL)
  {
    goto exit_and_cleanup;
  }

  self->functions.pvd_con_next = cape_dl_funct (self->hlib, "qbus_pvd_con_next", err);
  if (self->functions.pvd_con_next == NULL)
  {
    goto exit_and_cleanup;
  }

  // optional methods
  // if the library has its own initializing concept, those methods can be implemented
  self->functions.pvd_init = cape_dl_funct (self->hlib, "qbus_pvd_init", err);
  self->functions.pvd_done = cape_dl_funct (self->hlib, "qbus_pvd_done", err);

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "functions mapped = %s", engine_name);

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

QbusPvdCtx qbus_engine_ctx_new (QBusEngine engine, CapeAioContext aio, const CapeString name, CapeErr err)
{
  QbusPvdCtx self = NULL;

  if (engine->functions.pvd_ctx_new)
  {
    CapeUdc options = cape_udc_new (CAPE_UDC_NODE, NULL);

    if (name)
    {
      CapeString name_engine = cape_str_cp (name);

      cape_str_to_upper (name_engine);

      // always send the name in the options
      cape_udc_add_s_mv (options, "name", &name_engine);
    }

    // create a new engine context
    // -> might use the AIO for event handling
    // -> might use the options for config
    // -> contains all entities and connections
    self = engine->functions.pvd_ctx_new (aio, options, err);
    if (self)
    {
      // save the context in a list
      // can be destroyed later with the whole engine
      cape_list_push_back (engine->ctxs, qbus_engine_ctx_item_new (engine, self));
    }

    cape_udc_del (&options);
  }
  else
  {
    cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }

  return self;
}

//-----------------------------------------------------------------------------

void qbus_engine_ctx_add (QBusEngine engine, QbusPvdCtx self, QbusPvdConnection* p_con, CapeUdc options, void* user_ptr, fct_qbus_pvd__on_con on_con, fct_qbus_pvd__on_snd on_snd)
{
  if (engine->functions.pvd_ctx_add)
  {
    // create a new connection
    engine->functions.pvd_ctx_add (self, p_con, options, user_ptr, on_con, on_snd);
  }
}

//-----------------------------------------------------------------------------

int qbus_engine__lib__ctx_del (QBusEngine self, QbusPvdCtx* p_ctx, CapeErr err)
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

const CapeString qbus_engine_con_cid (QBusEngine self, QbusPvdConnection connection)
{
  if (self->functions.pvd_con_cid)
  {
    return self->functions.pvd_con_cid (connection);
  }
  else
  {
   // res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");

    return NULL;
  }
}

//-----------------------------------------------------------------------------

void qbus_engine_con_snd (QBusEngine self, QbusPvdConnection connection, const CapeString cid, QBusFrame frame)
{
  if (self->functions.pvd_con_snd)
  {
    self->functions.pvd_con_snd (connection, cid, frame);
  }
  else
  {
   // res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }
}

//-----------------------------------------------------------------------------

void qbus_engine_con_subscribe (QBusEngine self, QbusPvdConnection connection, const CapeString topic)
{
  if (self->functions.pvd_con_subscribe)
  {
    self->functions.pvd_con_subscribe (connection, topic);
  }
  else
  {
   // res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }
}

//-----------------------------------------------------------------------------

void qbus_engine_con_unsubscribe (QBusEngine self, QbusPvdConnection connection, const CapeString topic)
{
  if (self->functions.pvd_con_unsubscribe)
  {
    self->functions.pvd_con_unsubscribe (connection, topic);
  }
  else
  {
   // res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "qbus pvd interface was not initialized");
  }
}

//-----------------------------------------------------------------------------

void qbus_engine_con_next (QBusEngine self, QbusPvdConnection connection, const CapeString topic, QBusFrame frame)
{
  if (self->functions.pvd_con_next)
  {
    self->functions.pvd_con_next (connection, topic, frame);
  }
}

//-----------------------------------------------------------------------------

struct QBusEngines_s
{
  CapeString path;

  CapeMap engines;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_engines__engines__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusEngine h = val; qbus_engine_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusEngines qbus_engines_new (const CapeString engines_path)
{
  QBusEngines self = CAPE_NEW (struct QBusEngines_s);

  self->path = cape_str_cp (engines_path);
  self->engines = cape_map_new (cape_map__compare__s, qbus_engines__engines__on_del, NULL);

  return self;
}

//-----------------------------------------------------------------------------

void qbus_engines_del (QBusEngines* p_self)
{
  if (*p_self)
  {
    QBusEngines self = *p_self;

    cape_map_del (&(self->engines));
    cape_str_del (&(self->path));

    CAPE_DEL (p_self, struct QBusEngines_s);
  }
}

//-----------------------------------------------------------------------------

QBusEngine qbus_engines_add (QBusEngines self, const CapeString engine_name, CapeErr err)
{
  QBusEngine ret = NULL;

  CapeMapNode n = cape_map_find (self->engines, (void*)engine_name);

  if (n)
  {
    ret = cape_map_node_value (n);
  }
  else
  {
    ret = qbus_engine_new (self->path, engine_name, err);
    if (ret)
    {
      cape_map_insert (self->engines, (void*)cape_str_cp (engine_name), (void*)ret);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------
