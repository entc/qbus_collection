#include "qbus_engines_pvd.h"
#include "qbus_pvd.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_dl.h>
#include <sys/cape_file.h>
#include <fmt/cape_json.h>
#include <stc/cape_list.h>

//-----------------------------------------------------------------------------

struct QBusEnginesPvd_s
{
  CapeDl hlib;         // handle for the shared library
  CapeList ctxs;       // list of all contexts
  QBusPvd2 pvd2;
};

//-----------------------------------------------------------------------------

QBusEnginesPvd qbus_engines_pvd_new ()
{
  QBusEnginesPvd self = CAPE_NEW (struct QBusEnginesPvd_s);

  self->ctxs = cape_list_new (NULL);
  self->hlib = cape_dl_new ();

  return self;
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd_del (QBusEnginesPvd* p_self)
{
  if (*p_self)
  {
    QBusEnginesPvd self = *p_self;
    
    cape_log_msg (CAPE_LL_TRACE, "QBUS", "engines", "release engine");
    
    {
      CapeListCursor* cursor = cape_list_cursor_create (self->ctxs, CAPE_DIRECTION_FORW);
      
      while (cape_list_cursor_next (cursor))
      {
        QBusPvdCtx ctx = cape_list_cursor_extract (self->ctxs, cursor);
        
        self->pvd2.ctx_del (&ctx);
      }
      
      cape_list_cursor_destroy (&cursor);
    }
    
    cape_list_del (&(self->ctxs));
    cape_dl_del (&(self->hlib));
    
    CAPE_DEL (p_self, struct QBusEnginesPvd_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_engines_pvd_load (QBusEnginesPvd self, const CapeString path, const CapeString name, CapeErr err)
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
  
  self->pvd2.ctx_cb = cape_dl_funct (self->hlib, "pvd2_ctx_cb", err);
  if (self->pvd2.ctx_cb == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_str_del (&path_current);
  cape_str_del (&path_resolved);
  cape_str_del (&pvd_name);

  return res;
}

//-----------------------------------------------------------------------------

int qbus_engines_pvd__ctx_new (QBusEnginesPvd self, const CapeUdc config, CapeAioContext aio_context, CapeErr err)
{
  int res;
  
  QBusPvdCtx ctx = self->pvd2.ctx_new (config, aio_context, err);
  if (ctx == NULL)
  {
    res = cape_err_code (err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "no pvd context returned");
    goto exit_and_cleanup;
  }
  
  cape_list_push_back (self->ctxs, (void*)ctx);
  
  self->pvd2.ctx_cb (ctx);
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------
